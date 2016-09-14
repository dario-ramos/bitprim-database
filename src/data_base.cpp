/**
 * Copyright (c) 2011-2015 libbitcoin developers (see AUTHORS)
 *
 * This file is part of libbitcoin.
 *
 * libbitcoin is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License with
 * additional permissions to the one published by the Free Software
 * Foundation, either version 3 of the License, or (at your option)
 * any later version. For more information see LICENSE.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#include <bitcoin/database/data_base.hpp>

#include <cstdint>
#include <cstddef>
#include <memory>
#include <stdexcept>
#include <boost/filesystem.hpp>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/memory/memory_map.hpp>
#include <bitcoin/database/settings.hpp>

namespace libbitcoin {
namespace database {

using namespace boost::filesystem;
using namespace bc::chain;
using namespace bc::wallet;

// BIP30 exception blocks.
// github.com/bitcoin/bips/blob/master/bip-0030.mediawiki#specification
static const config::checkpoint exception1 =
{ "00000000000a4d0a398161ffc163c503763b1f4360639393e0e4c8e300e0caec", 91842 };
static const config::checkpoint exception2 =
{ "00000000000743f190a18c5577a3c2d2a1f610ae9601ac046a38084ccb7cd721", 91880 };

bool data_base::touch_file(const path& file_path)
{
    bc::ofstream file(file_path.string());
    if (file.bad())
        return false;

    // Write one byte so file is nonzero size.
    file.write("X", 1);
    return true;
}

bool data_base::initialize(const path& prefix, const chain::block& genesis)
{
    // Create paths.
    const store paths(prefix);

    if (!paths.touch_all())
        return false;

    data_base instance(paths, 0, 0);

    if (!instance.create())
        return false;

    instance.push(genesis);
    return instance.stop();
}

data_base::store::store(const path& prefix)
{
    // Hash-based lookup (hash tables).
    blocks_lookup = prefix / "block_table";
    history_lookup = prefix / "history_table";
    spends_lookup = prefix / "spend_table";
    unspents_lookup = prefix / "unspent_table";
    transactions_lookup = prefix / "transaction_table";

    // Height-based (reverse) lookup.
    blocks_index = prefix / "block_index";

    // One (address) to many (rows).
    history_rows = prefix / "history_rows";
    stealth_rows = prefix / "stealth_rows";

    // Exclusive database access reserved by this process.
    database_lock = prefix / "process_lock";
}

bool data_base::store::touch_all() const
{
    // Return the result of the database file create.
    return
        touch_file(blocks_lookup) &&
        touch_file(blocks_index) &&
        touch_file(history_lookup) &&
        touch_file(history_rows) &&
        touch_file(stealth_rows) &&
        touch_file(spends_lookup) &&
        touch_file(unspents_lookup) &&
        touch_file(transactions_lookup);
}

data_base::file_lock data_base::initialize_lock(const path& lock)
{
    // Touch the lock file to ensure its existence.
    const auto lock_file_path = lock.string();
    bc::ofstream file(lock_file_path, std::ios::app);
    file.close();

    // BOOST:
    // Opens a file lock. Throws interprocess_exception if the file does not
    // exist or there are no operating system resources. The file lock is
    // destroyed on its destruct and does not throw.
    return file_lock(lock_file_path.c_str());
}

void data_base::uninitialize_lock(const path& lock)
{
    // BUGBUG: Throws if the lock is not held (i.e. in error condition).
    boost::filesystem::remove(lock);
}

data_base::data_base(const settings& settings)
  : data_base(settings.directory, settings.history_start_height,
        settings.stealth_start_height)
{
}

data_base::data_base(const path& prefix, size_t history_height,
    size_t stealth_height)
  : data_base(store(prefix), history_height, stealth_height)
{
}

data_base::data_base(const store& paths, size_t history_height,
    size_t stealth_height)
  : lock_file_path_(paths.database_lock),
    history_height_(history_height),
    stealth_height_(stealth_height),
    sequential_lock_(0),
    mutex_(std::make_shared<shared_mutex>()),
    blocks(paths.blocks_lookup, paths.blocks_index, mutex_),
    history(paths.history_lookup, paths.history_rows, mutex_),
    stealth(paths.stealth_rows, mutex_),
    spends(paths.spends_lookup, mutex_),
    unspents(paths.unspents_lookup, mutex_),
    transactions(paths.transactions_lookup, mutex_)
{
}

// Close does not call stop because there is no way to detect thread join.
data_base::~data_base()
{
    close();
}

// Startup and shutdown.
// ----------------------------------------------------------------------------

// Leaves database in started state.
// Throws if there is insufficient disk space.
// TODO: merge this with file creation (initialization above).
// This is actually first initialization of existing files, not file creation.
bool data_base::create()
{
    // Return the result of the database create.
    return 
        blocks.create() &&
        history.create() &&
        spends.create() &&
        unspents.create() &&
        stealth.create() &&
        transactions.create();
}

// Start must be called before performing queries.
// Start may be called after stop and/or after close in order to restart.
bool data_base::start()
{
    // TODO: create a class to encapsulate the full file lock concept.
    file_lock_ = std::make_shared<file_lock>(initialize_lock(lock_file_path_));

    // BOOST:
    // Effects: The calling thread tries to acquire exclusive ownership of the
    // mutex without waiting. If no other thread has exclusive, or sharable
    // ownership of the mutex this succeeds. Returns: If it can acquire
    // exclusive ownership immediately returns true. If it has to wait, returns
    // false. Throws: interprocess_exception on error. Note that a file lock
    // can't guarantee synchronization between threads of the same process so
    // just use file locks to synchronize threads from different processes.
    if (!file_lock_->try_lock())
        return false;

    const auto start_exclusive = begin_write();
    const auto start_result =
        blocks.start() &&
        history.start() &&
        spends.start() &&
        unspents.start() &&
        stealth.start() &&
        transactions.start();
    const auto end_exclusive = end_write();

    // Return the result of the database start.
    return start_exclusive && start_result && end_exclusive;
}

// Stop only accelerates work termination, only required if restarting.
bool data_base::stop()
{
    const auto start_exclusive = begin_write();
    const auto blocks_stop = blocks.stop();
    const auto history_stop = history.stop();
    const auto spends_stop = spends.stop();
    const auto unspents_stop = unspents.stop();
    const auto stealth_stop = stealth.stop();
    const auto transactions_stop = transactions.stop();
    const auto end_exclusive = end_write();

    // This should remove the lock file. This is not important for locking
    // purposes, but it provides a sentinel to indicate hard shutdown.
    file_lock_ = nullptr;
    uninitialize_lock(lock_file_path_);

    // Return the cumulative result of the database shutdowns.
    return
        start_exclusive &&
        blocks_stop &&
        history_stop &&
        spends_stop &&
        unspents_stop &&
        stealth_stop &&
        transactions_stop &&
        end_exclusive;
}

// Close is optional as the database will close on destruct.
bool data_base::close()
{
    const auto blocks_close = blocks.close();
    const auto history_close = history.close();
    const auto spends_close = spends.close();
    const auto unspents_close = unspents.close();
    const auto stealth_close = stealth.close();
    const auto transactions_close = transactions.close();

    // Return the cumulative result of the database closes.
    return
        blocks_close &&
        history_close &&
        spends_close &&
        unspents_close &&
        stealth_close &&
        transactions_close;
}

// Locking.
// ----------------------------------------------------------------------------

handle data_base::begin_read()
{
    return sequential_lock_.load();
}

bool data_base::is_read_valid(handle value)
{
    return value == sequential_lock_.load();
}

bool data_base::is_write_locked(handle value)
{
    return (value % 2) == 1;
}

// TODO: drop a file as a write sentinel that we can use to detect uncontrolled
// shutdown during write. Use a similar approach around initial block download.
// Fail startup if the sentinel is detected. (file: write_lock).
bool data_base::begin_write()
{
    // slock is now odd.
    return is_write_locked(++sequential_lock_);
}

// TODO: clear the write sentinel.
bool data_base::end_write()
{
    // slock_ is now even again.
    return !is_write_locked(++sequential_lock_);
}

// Query engines.
// ----------------------------------------------------------------------------

static size_t get_next_height(const block_database& blocks)
{
    size_t current_height;
    const auto empty_chain = !blocks.top(current_height);
    return empty_chain ? 0 : current_height + 1;
}

static bool is_allowed_duplicate(const header& head, size_t height)
{
    return
        (height == exception1.height() && head.hash() == exception1.hash()) ||
        (height == exception2.height() && head.hash() == exception2.hash());
}

void data_base::synchronize()
{
    spends.sync();
    unspents.sync();
    history.sync();
    stealth.sync();
    transactions.sync();
    blocks.sync();
}

void data_base::push(const block& block)
{
    // Height is unsafe unless database locked.
    push(block, get_next_height(blocks));
}

void data_base::push(const block& block, uint64_t height)
{
    for (size_t index = 0; index < block.transactions.size(); ++index)
    {
        // Skip BIP30 allowed duplicates (coinbase txs of excepted blocks).
        // We handle here because this is the lowest public level exposed.
        if (index == 0 && is_allowed_duplicate(block.header, height))
            continue;

        const auto& tx = block.transactions[index];
        const auto tx_hash = tx.hash();

        // Add inputs
        if (!tx.is_coinbase())
            push_inputs(tx_hash, height, tx.inputs);

        // Add outputs
        push_outputs(tx_hash, height, tx.outputs);

        // Add stealth outputs
        push_stealth(tx_hash, height, tx.outputs);

        // Add transaction
        transactions.store(height, index, tx);
    }

    // Add block itself.
    blocks.store(block, height);

    // Synchronise everything that was added.
    synchronize();
}

void data_base::push_inputs(const hash_digest& tx_hash, size_t height,
    const input::list& inputs)
{
    std::cout << "void data_base::push_inputs(const hash_digest& tx_hash, size_t height, const input::list& inputs)\n";

    for (uint32_t index = 0; index < inputs.size(); ++index)
    {
        // We also push spends in the inputs loop.
        const auto& input = inputs[index];
        const chain::input_point point{ tx_hash, index };
        spends.store(input.previous_output, point);
        unspents.remove(input.previous_output);

        if (height < history_height_)
            continue;

        // Try to extract an address.
        const auto address = payment_address::extract(input.script);
        if (!address)
            continue;

        const auto& previous = input.previous_output;
        history.add_input(address.hash(), point, height, previous);
    }
}

void data_base::push_outputs(const hash_digest& tx_hash, size_t height,
    const output::list& outputs)
{
    if (height < history_height_)
        return;

    for (uint32_t index = 0; index < outputs.size(); ++index)
    {
        const auto& output = outputs[index];
        const chain::output_point point{ tx_hash, index };

        unspents.store(point);

        // Try to extract an address.
        const auto address = payment_address::extract(output.script);
        if (!address)
            continue;

        const auto value = output.value;
        history.add_output(address.hash(), point, height, value);
    }
}

void data_base::push_stealth(const hash_digest& tx_hash, size_t height,
    const output::list& outputs)
{
    if (height < stealth_height_ || outputs.empty())
        return;

    // Stealth outputs are paired by convention.
    for (size_t index = 0; index < (outputs.size() - 1); ++index)
    {
        const auto& ephemeral_script = outputs[index].script;
        const auto& payment_script = outputs[index + 1].script;

        // Try to extract an unsigned ephemeral key from the first output.
        hash_digest unsigned_ephemeral_key;
        if (!extract_ephemeral_key(unsigned_ephemeral_key, ephemeral_script))
            continue;

        // Try to extract a stealth prefix from the first output.
        uint32_t prefix;
        if (!to_stealth_prefix(prefix, ephemeral_script))
            continue;

        // Try to extract the payment address from the second output.
        const auto address = payment_address::extract(payment_script);
        if (!address)
            continue;

        // The payment address versions are arbitrary and unused here.
        const chain::stealth_compact row
        {
            unsigned_ephemeral_key,
            address.hash(),
            tx_hash
        };

        stealth.store(prefix, height, row);
    }
}

chain::block data_base::pop()
{
    size_t height;
    DEBUG_ONLY(const auto result =) blocks.top(height);
    BITCOIN_ASSERT_MSG(result, "Pop on empty database.");

    const auto block_result = blocks.get(height);
    const auto count = block_result.transaction_count();

    // Build the block for return.
    chain::block block;
    block.header = block_result.header();
    block.transactions.reserve(count);
    auto& txs = block.transactions;

    for (auto tx = 0; tx < count; ++tx)
    {
        const auto tx_hash = block_result.transaction_hash(tx);
        const auto tx_result = transactions.get(tx_hash);

        BITCOIN_ASSERT(tx_result);
        BITCOIN_ASSERT(tx_result.height() == height);
        BITCOIN_ASSERT(tx_result.index() == static_cast<size_t>(tx));

        // TODO: the deserialization should cache the hash on the tx.
        // Deserialize the transaction and move it to the block.
        block.transactions.emplace_back(tx_result.transaction());
    }

    // Loop txs backwards, the reverse of how they are added.
    // Remove txs, then outputs, then inputs (also reverse order).
    for (auto tx = txs.rbegin(); tx != txs.rend(); ++tx)
    {
        transactions.remove(tx->hash());
        // pop_outputs(tx->outputs, height);
        pop_outputs(tx->hash(), height, tx->outputs);

        if (!tx->is_coinbase())
            pop_inputs(tx->inputs, height);
    }

    // Stealth unlink is not implemented.
    stealth.unlink(height);
    blocks.unlink(height);

    // Synchronise everything that was changed.
    synchronize();

    // Return the block.
    return block;
}

void data_base::pop_inputs(const input::list& inputs, size_t height)
{
    // Loop in reverse.
    for (auto input = inputs.rbegin(); input != inputs.rend(); ++input)
    {
        spends.remove(input->previous_output);
        unspents.store(input->previous_output);

        if (height < history_height_)
            continue;

        // Try to extract an address.
        const auto address = payment_address::extract(input->script);

        if (address)
            history.delete_last_row(address.hash());
    }
}

// void data_base::pop_outputs(const output::list& outputs, size_t height)
void data_base::pop_outputs(hash_digest const& tx_hash, size_t height,
    output::list const& outputs)
{
    //TODO Fer: cuando se llama a esto?
    std::cout << "FER -- void data_base::pop_outputs(hash_digest const& tx_hash, size_t height, output::list const& outputs)\n";

    if (height < history_height_)
        return;

    // Loop in reverse.
    // for (auto output = outputs.rbegin(); output != outputs.rend(); ++output)
    // {
    //     const chain::output_point point{ tx_hash, index };
    //     unspents.remove(*output);

    //     // Try to extract an address.
    //     const auto address = payment_address::extract(output->script);

    //     if (address)
    //         history.delete_last_row(address.hash());
    // }

    // Loop in reverse.
    for (uint32_t i = outputs.size(); i-- > 0;) {
        const auto& output = outputs[i];

        const chain::output_point point {tx_hash, i};
        unspents.remove(point);

        // Try to extract an address.
        const auto address = payment_address::extract(output.script);

        if (address)
            history.delete_last_row(address.hash());
    }    
}

} // namespace data_base
} // namespace libbitcoin
