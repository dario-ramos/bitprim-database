/**
 * Copyright (c) 2016 Bitprim developers (see AUTHORS)
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
#include <bitcoin/database/databases/transaction_database.hpp>

#include <cstddef>
#include <cstdint>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/result/transaction_result.hpp>

namespace libbitcoin {
namespace database {

using namespace bc::chain;

static const auto use_wire_encoding = false;
static constexpr auto value_size = sizeof(uint64_t);
static constexpr auto height_size = sizeof(uint32_t);
static constexpr auto version_size = sizeof(uint32_t);
static constexpr auto locktime_size = sizeof(uint32_t);
static constexpr auto position_size = sizeof(uint32_t);
static constexpr auto version_lock_size = version_size + locktime_size;

static constexpr char insert_tx_sql[] = "INSERT INTO transactions (hash, version, locktime) VALUES (?1, ?2, ?3);";
static constexpr char insert_txin_sql[] = "INSERT INTO input (transaction_id, prev_output_hash, prev_output_index, script, sequence) VALUES (?1, ?2, ?3, ?4, ?5);";
static constexpr char insert_txout_sql[] = "INSERT INTO output (transaction_id, idx, amount, script) VALUES (?1, ?2, ?3, ?4);";
static constexpr char select_tx_sql[] = "SELECT id, version, locktime FROM transactions WHERE hash = ?1 ORDER BY id;";
static constexpr char select_txin_sql[] = "SELECT id, prev_output_hash, prev_output_index, script, sequence FROM input WHERE transaction_id = ?1 ORDER BY id;";
static constexpr char select_txout_sql[] = "SELECT id, idx, amount, script FROM output WHERE transaction_id = ?1 ORDER BY id;";

// Transactions uses a hash table index, O(1).
transaction_database::transaction_database(path const& filename)
    : tx_db(filename.c_str())
    // , insert_tx_stmt_()
    // , insert_tx_input_stmt_()
    // , insert_tx_output_stmt_()
    // , select_tx_by_hash_stmt_()
    // , select_txin_by_txid_stmt_()
    // , select_txout_by_txid_stmt_()
{
    int rc;
    
    rc = sqlite3_prepare_v2(db.ptr(), insert_tx_sql, -1, &insert_tx_stmt_, NULL);
    rc = sqlite3_prepare_v2(db.ptr(), insert_txin_sql, -1, &insert_tx_input_stmt_, NULL);
    rc = sqlite3_prepare_v2(db.ptr(), insert_txout_sql, -1, &insert_tx_output_stmt_, NULL);

    rc = sqlite3_prepare_v2(db.ptr(), select_tx_sql, -1, &select_tx_by_hash_stmt_, NULL);
    rc = sqlite3_prepare_v2(db.ptr(), select_txin_sql, -1, &select_txin_by_txid_stmt_, NULL);
    rc = sqlite3_prepare_v2(db.ptr(), select_txout_sql, -1, &select_txout_by_txid_stmt_, NULL);

    //TODO: Fer: check for errors
}

transaction_database::~transaction_database() {
    close();
}

// Create.
// ----------------------------------------------------------------------------

// Initialize files and start.
bool transaction_database::create() {

    bool res = true;

    sql = "CREATE TABLE transactions( "
            "id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, "
            "hash TEXT NOT NULL UNIQUE, "
            "version INTEGER NOT NULL, "
            "locktime INTEGER NOT NULL );";

    // db.exec(sql, [](int res, std::string const& error_msg){
    //     if (res != SQLITE_OK) {
    //         fprintf(stderr, "Create SQL error: %s\n", error_msg.c_str());
    //     } else {
    //         fprintf(stdout, "Table created successfully\n");
    //     }
    // });

    db.exec(sql, [](int res, std::string const& error_msg){
        if (res != SQLITE_OK) {
            //TODO: Fer: Log errors
            res = false;
        }
    });
    if (!res) return res;

    sql = "CREATE TABLE input ("
            "`id` INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, "
            "`transaction_id` INTEGER NOT NULL, "
            "`prev_output_hash` TEXT NOT NULL, "
            "`prev_output_index` INTEGER NOT NULL, "
            "`script` BLOB, "
            "`sequence` INTEGER NOT NULL );";

    db.exec(sql, [](int res, std::string const& error_msg){
        if (res != SQLITE_OK) {
            res = false;
        }
    });
    if (!res) return res;

    sql = "CREATE TABLE output ( "
            "id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, "
            "transaction_id INTEGER NOT NULL, "
            "idx INTEGER NOT NULL, "
            "amount INTEGER NOT NULL, "
            "script BLOB );";

    db.exec(sql, [](int res, std::string const& error_msg){
        if (res != SQLITE_OK) {
            res = false;
        }
    });
    

    return res;

//    // Resize and create require an opened file.
//    if (!lookup_file_.open())
//        return false;
//
//    // This will throw if insufficient disk space.
//    lookup_file_.resize(initial_map_file_size_);
//
//    if (!lookup_header_.create() ||
//        !lookup_manager_.create())
//        return false;
//
//    // Should not call start after create, already started.
//    return
//        lookup_header_.start() &&
//        lookup_manager_.start();
}

// Startup and shutdown.
// ----------------------------------------------------------------------------

// Start files and primitives.
bool transaction_database::open() {
//    return
//        lookup_file_.open() &&
//        lookup_header_.start() &&
//        lookup_manager_.start();
}

// Close files.
bool transaction_database::close() {
//    return lookup_file_.close();
}

// Commit latest inserts.
void transaction_database::synchronize() {
//    lookup_manager_.sync();
}

// Flush the memory map to disk.
bool transaction_database::flush() {
//    return lookup_file_.flush();
}

// Queries.
// ----------------------------------------------------------------------------

transaction_result transaction_database::get(hash_digest const& hash) const {
    return get(hash, max_size_t);
}

chain::input::list select_inputs(sqlite3* db, sqlite3_stmt* stmt, int64_t tx_id) {
    sqlite3_bind_int64(stmt, 1, tx_id);

    chain::input::list res;

    int rc;
    while ( (rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        // auto id = sqlite3_column_int64(stmt, 0);

        hash_digest prev_output_hash;
        memcpy(prev_output_hash.data(), sqlite3_column_text(stmt, 1), sizeof(prev_output_hash));
        auto prev_output_index = static_cast<uint32_t>(sqlite3_column_int(stmt, 2));

        // output_point(hash_digest&& hash, uint32_t index);
        // output_point(const hash_digest& hash, uint32_t index);
        chain::output_point previous_output(prev_output_hash, prev_output_index);

        auto script_size = sqlite3_column_bytes(stmt, 3);
        auto script_ptr = static_cast<uint8_t const*>(sqlite3_column_blob(stmt, 3));
        std::vector<uint8_t> script_data(script_ptr, script_ptr + script_size);
        chain::script::factory_from_data script(script_data, true);

        auto sequence = static_cast<uint32_t>(sqlite3_column_int(stmt, 1));

        // input(output_point&& previous_output, chain::script&& script, uint32_t sequence);
        // input(const output_point& previous_output, const chain::script& script, uint32_t sequence);
        chain::input in(previous_output, script, sequence);

        // txin_query_record rec;
        // rec.id = id;
        // rec.prev_output_hash = prev_output_hash;
        // rec.prev_output_index = prev_output_index;
        // rec.script = script;
        // rec.sequence = sequence;
        res.push_back(in);

        //TODO: implement the constructor
//        res.emplace_back(id, prev_output_hash, prev_output_index, script, sequence);
    }

    sqlite3_reset(stmt);

    return std::make_pair(true, res);
}

//sql = "SELECT id, index, amount, script FROM output WHERE transaction_id = ?1 ORDER BY id;";
chain::output::list select_outputs(sqlite3* db, sqlite3_stmt* stmt, int64_t tx_id) {
    sqlite3_bind_int64(stmt, 1, tx_id);

    chain::output::list res;

    int rc;
    while ( (rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        auto id = sqlite3_column_int64(stmt, 0);
        auto index = static_cast<uint32_t>(sqlite3_column_int(stmt, 1));
        auto amount = static_cast<uint64_t>(sqlite3_column_int64(stmt, 2));

        auto script_size = sqlite3_column_bytes(stmt, 3);
        auto script_ptr = static_cast<uint8_t const*>(sqlite3_column_blob(stmt, 3));
        std::vector<uint8_t> script_data(script_ptr, script_ptr + script_size);

        chain::script::factory_from_data script(script_data, true);

        chain::output out(amount, script);

        // txout_query_record rec;
        // rec.id = id;
        // rec.index = index;
        // rec.amount = amount;
        // rec.script = script;

        res.push_back(out);

        //TODO: implement the constructor
//        res.emplace_back(id, index, amount, script);
    }

    sqlite3_reset(stmt);

    return std::make_pair(true, res);
}


transaction_result transaction_database::get(hash_digest const& hash, size_t /*DEBUG_ONLY(fork_height)*/) const {
    // TODO: use lookup_map_ to search a set of transactions in height order,
    // returning the highest that is at or below the specified fork height.
    // Short-circuit the search if fork_height is max_size_t (just get first).
    ////BITCOIN_ASSERT_MSG(fork_height == max_size_t, "not implemented");
//    const auto memory = lookup_map_.find(hash);
//    return transaction_result(memory, hash);


    // tx_db.exec_each("SELECT * FROM transactions WHERE ????", [](int argc, char** argv, char** azColName){
    //     for (size_t i = 0; i < argc; ++i) {
    //         printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    //     }
    //     return 0;
    // }, [](int res, std::string const& error_msg){
    //     if (res != SQLITE_OK) {
    //         fprintf(stderr, "SQL error: %s\n", error_msg);
    //     } else {
    //         fprintf(stdout, "Select done successfully\n");
    //     }
    // });

    sqlite3_bind_text(select_tx_by_hash_stmt_, 1, reinterpret_cast<char const*>(hash.data()), sizeof(hash_digest), SQLITE_STATIC);

    int rc = sqlite3_step(select_tx_by_hash_stmt_);
    if (rc == SQLITE_ROW) {
        auto id = sqlite3_column_int64(select_tx_by_hash_stmt_, 0);
        auto version = static_cast<uint32_t>(sqlite3_column_int(select_tx_by_hash_stmt_, 1));
        auto locktime = static_cast<uint32_t>(sqlite3_column_int(select_tx_by_hash_stmt_, 2));

        // input::list select_inputs(sqlite3* db, sqlite3_stmt* stmt, int64_t tx_id) {
        auto inputs = select_inputs(tx_db.ptr(), select_txin_by_txid_stmt_, id);
        auto outputs = select_outputs(tx_db.ptr(), select_txout_by_txid_stmt_, id);


        // transaction(uint32_t version, uint32_t locktime, ins&& inputs, outs&& outputs);
        // transaction(uint32_t version, uint32_t locktime, const ins& inputs,
        //     const outs& outputs);

        chain::transaction tx(version, locktime, inputs, outputs);
        // tx.from_data(deserial, use_wire_encoding);

        // TODO: add hash param to deserialization to eliminate this construction.
        return chain::transaction(std::move(tx), hash_digest(hash));


        sqlite3_reset(select_tx_by_hash_stmt_);
        return std::make_pair(true, transaction_query_result {id, version, locktime});

    } else {
        sqlite3_reset(select_tx_by_hash_stmt_);
        return std::make_pair(true, transaction_query_result());
    }

    return transaction_result(true);
}

bool transaction_database::update(output_point const& point, size_t spender_height) {
//    const auto slab = lookup_map_.find(point.hash());
//    const auto memory = REMAP_ADDRESS(slab);
//    const auto tx_start = memory + height_size + position_size;
//    auto serial = make_unsafe_serializer(tx_start);
//    serial.skip(version_size + locktime_size);
//    const auto outputs = serial.read_size_little_endian();
//    BITCOIN_ASSERT(serial);
//
//    if (point.index() >= outputs)
//        return false;
//
//    // Skip outputs until the target output.
//    for (uint32_t output = 0; output < point.index(); ++output)
//    {
//        serial.skip(height_size);
//        serial.skip(value_size);
//        serial.skip(serial.read_size_little_endian());
//        BITCOIN_ASSERT(serial);
//    }
//
//    // Write the spender height to the first word of the target output.
//    serial.write_4_bytes_little_endian(spender_height);
//    return true;
}

void transaction_database::store(size_t height, size_t position, chain::transaction const& tx) {
//    // Write block data.
//    const auto key = tx.hash();
//    const auto tx_size = tx.serialized_size(false);
//
//    BITCOIN_ASSERT(height <= max_uint32);
//    const auto hight32 = static_cast<size_t>(height);
//
//    BITCOIN_ASSERT(position <= max_uint32);
//    const auto position32 = static_cast<size_t>(position);
//
//    BITCOIN_ASSERT(tx_size <= max_size_t - version_lock_size);
//    const auto value_size = version_lock_size + static_cast<size_t>(tx_size);
//
//    const auto write = [&hight32, &position32, &tx](memory_ptr data)
//    {
//        auto serial = make_unsafe_serializer(REMAP_ADDRESS(data));
//        serial.write_4_bytes_little_endian(hight32);
//        serial.write_4_bytes_little_endian(position32);
//
//        // WRITE THE TX
//        serial.write_bytes(tx.to_data(use_wire_encoding));
//    };
//
//    lookup_map_.store(key, write, value_size);
}

bool transaction_database::unlink(hash_digest const& hash) {
//    return lookup_map_.unlink(hash);
}

} // namespace database
} // namespace libbitcoin
