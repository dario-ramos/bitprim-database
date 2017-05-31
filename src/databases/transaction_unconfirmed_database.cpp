/**
 * Copyright (c) 2011-2017 libbitcoin developers (see AUTHORS)
 *
 * This file is part of libbitcoin.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <bitcoin/database/databases/transaction_unconfirmed_database.hpp>

#include <cstddef>
#include <cstdint>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/result/transaction_result.hpp>

namespace libbitcoin {
namespace database {

using namespace bc::chain;
using namespace bc::machine;

static constexpr auto value_size = sizeof(uint64_t);
static constexpr auto version_size = sizeof(uint32_t);
static constexpr auto locktime_size = sizeof(uint32_t);
static constexpr auto version_lock_size = version_size + locktime_size;

const size_t transaction_unconfirmed_database::unconfirmed = max_uint32;

// Transactions uses a hash table index, O(1).
transaction_unconfirmed_database::transaction_unconfirmed_database(const path& map_filename,
    size_t buckets, size_t expansion, mutex_ptr mutex)
  : initial_map_file_size_(slab_hash_table_header_size(buckets) + minimum_slabs_size),
    lookup_file_(map_filename, mutex, expansion),
    lookup_header_(lookup_file_, buckets),
    lookup_manager_(lookup_file_, slab_hash_table_header_size(buckets)),
    lookup_map_(lookup_header_, lookup_manager_)
{}

transaction_unconfirmed_database::~transaction_unconfirmed_database()
{
    close();
}

// Create.
// ----------------------------------------------------------------------------

// Initialize files and start.
bool transaction_unconfirmed_database::create()
{
    // Resize and create require an opened file.
    if (!lookup_file_.open())
        return false;

    // This will throw if insufficient disk space.
    lookup_file_.resize(initial_map_file_size_);

    if (!lookup_header_.create() ||
        !lookup_manager_.create())
        return false;

    // Should not call start after create, already started.
    return
        lookup_header_.start() &&
        lookup_manager_.start();
}

// Startup and shutdown.
// ----------------------------------------------------------------------------

// Start files and primitives.
bool transaction_unconfirmed_database::open()
{
    return
        lookup_file_.open() &&
        lookup_header_.start() &&
        lookup_manager_.start();
}

// Close files.
bool transaction_unconfirmed_database::close()
{
    return lookup_file_.close();
}

// Commit latest inserts.
void transaction_unconfirmed_database::synchronize()
{
    lookup_manager_.sync();
}

// Flush the memory map to disk.
bool transaction_unconfirmed_database::flush() const
{
    return lookup_file_.flush();
}

// Queries.
// ----------------------------------------------------------------------------
//OLD MERGE MAY 2017
//memory_ptr transaction_unconfirmed_database::find(const hash_digest& hash) const
//{
//    //*************************************************************************
//    // CONSENSUS: This simplified implementation does not allow the possibility
//    // of a matching tx hash above the fork height or the existence of both
//    // unconfirmed and confirmed transactions with the same hash. This is an
//    // assumption of the impossibility of hash collisions, which is incorrect
//    // but consistent with the current satoshi implementation. This method
//    // encapsulates that assumption which can therefore be fixed in one place.
//    //*************************************************************************
//    auto slab = lookup_map_.find(hash);
//    return slab;
//}

memory_ptr transaction_unconfirmed_database::find(const hash_digest& hash,
                                      size_t fork_height, bool require_confirmed) const
{
    //*************************************************************************
    // CONSENSUS: This simplified implementation does not allow the possibility
    // of a matching tx hash above the fork height or the existence of both
    // unconfirmed and confirmed transactions with the same hash. This is an
    // assumption of the impossibility of hash collisions, which is incorrect
    // but consistent with the current satoshi implementation. This method
    // encapsulates that assumption which can therefore be fixed in one place.
    //*************************************************************************
    auto slab = lookup_map_.find(hash /*, fork_height, require_confirmed*/);
    return slab;
}


//OLD MERGE MAY 2017
//transaction_result transaction_unconfirmed_database::get(const hash_digest& hash,
//    size_t fork_height, bool require_confirmed) const
//{
//    // Limit search to confirmed transactions at or below the fork height.
//    // Caller should set fork height to max_size_t for unconfirmed search.
//    const auto slab = find(hash);
//
//    // Returns an invalid result if not found.
//    return transaction_result(slab, hash);
//}

//TODO: check if transaction_uncofirmed_get was called from mining
transaction_result transaction_unconfirmed_database::get(const hash_digest& hash,
     size_t fork_height, bool require_confirmed) const
{
    // Limit search to confirmed transactions at or below the fork height.
    // Caller should set fork height to max_size_t for unconfirmed search.
    const auto slab = find(hash, fork_height, require_confirmed);
    if (slab) {
        metadata_mutex_.lock_shared();
        auto deserial = make_unsafe_deserializer(REMAP_ADDRESS(slab));
        const auto height = deserial.read_4_bytes_little_endian();
        const auto position = deserial.read_4_bytes_little_endian();
        metadata_mutex_.unlock_shared();
        return transaction_result(slab, hash, fork_height, position);
    }

    return {};
}


void transaction_unconfirmed_database::store(const chain::transaction& tx)
{
    const auto hash = tx.hash();

    // Unconfirmed txs: position is unconfirmed and height is validation forks.
    const auto write = [&](serializer<uint8_t*>& serial)
    {
        serial.write_4_bytes_little_endian(static_cast<size_t>(0));
        serial.write_4_bytes_little_endian(static_cast<size_t>(unconfirmed));

        // WRITE THE TX
        tx.to_data(serial, false);
    };

    const auto tx_size = tx.serialized_size(false);
    BITCOIN_ASSERT(tx_size <= max_size_t - version_lock_size);
    const auto value_size = version_lock_size + static_cast<size_t>(tx_size);

    // Create slab for the new tx instance.
    lookup_map_.store(hash, write, value_size);
}

// bool transaction_unconfirmed_database::spend(const output_point& point, size_t spender_height)
// bool transaction_unconfirmed_database::unspend(const output_point& point)
// bool transaction_unconfirmed_database::confirm(const hash_digest& hash, size_t height, size_t position)
// bool transaction_unconfirmed_database::unconfirm(const hash_digest& hash)

bool transaction_unconfirmed_database::unlink(hash_digest const& hash) {
    return lookup_map_.unlink(hash);
}

bool transaction_unconfirmed_database::unlink_if_exists(hash_digest const& hash) {
    auto memory = lookup_map_.find(hash);
    if (memory == nullptr)
        return false;

    // Release lock.
    memory = nullptr;
    return unlink(hash);
}


} // namespace database
} // namespace libbitcoin
