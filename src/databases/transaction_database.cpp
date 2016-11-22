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
//static constexpr auto value_size = sizeof(uint64_t);
//static constexpr auto height_size = sizeof(uint32_t);
//static constexpr auto version_size = sizeof(uint32_t);
//static constexpr auto locktime_size = sizeof(uint32_t);
//static constexpr auto position_size = sizeof(uint32_t);
//static constexpr auto version_lock_size = version_size + locktime_size;

static constexpr char insert_tx_sql[] = "INSERT INTO transactions (hash, version, locktime, block_height, position) VALUES (?1, ?2, ?3, ?4, ?5);";
static constexpr char insert_txin_sql[] = "INSERT INTO input (transaction_id, prev_output_hash, prev_output_index, script, sequence) VALUES (?1, ?2, ?3, ?4, ?5);";
static constexpr char insert_txout_sql[] = "INSERT INTO output (transaction_id, idx, amount, script, spender_height) VALUES (?1, ?2, ?3, ?4, ?5);";

// static constexpr char select_tx_sql[] = "SELECT id, version, locktime, block_height, position FROM transactions WHERE hash = ?1 ORDER BY id;";
static constexpr char select_tx_sql[] = "SELECT id, version, locktime, block_height, position FROM transactions WHERE hash = ?1;";

static constexpr char select_txin_sql[] = "SELECT id, prev_output_hash, prev_output_index, script, sequence FROM input WHERE transaction_id = ?1 ORDER BY id;";
static constexpr char select_txout_sql[] = "SELECT id, idx, amount, script, spender_height FROM output WHERE transaction_id = ?1 ORDER BY id;";

// static constexpr char update_txout_sql[] = "UPDATE output SET spender_height = ?1 WHERE id = ?2;";
static constexpr char update_txout_sql[] = "UPDATE output SET spender_height = ?1 WHERE transaction_id = ?2 AND index = ?3;";

static constexpr char delete_tx_sql[] = "DELETE FROM transactions WHERE id = ?1;";
static constexpr char delete_txin_sql[] = "DELETE FROM input WHERE transaction_id = ?1;";
static constexpr char delete_txout_sql[] = "DELETE FROM output WHERE transaction_id = ?1;";


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
    
    rc = sqlite3_prepare_v2(tx_db.ptr(), insert_tx_sql, -1, &insert_tx_stmt_, NULL);
    rc = sqlite3_prepare_v2(tx_db.ptr(), insert_txin_sql, -1, &insert_tx_input_stmt_, NULL);
    rc = sqlite3_prepare_v2(tx_db.ptr(), insert_txout_sql, -1, &insert_tx_output_stmt_, NULL);

    rc = sqlite3_prepare_v2(tx_db.ptr(), select_tx_sql, -1, &select_tx_by_hash_stmt_, NULL);
    rc = sqlite3_prepare_v2(tx_db.ptr(), select_txin_sql, -1, &select_txin_by_txid_stmt_, NULL);
    rc = sqlite3_prepare_v2(tx_db.ptr(), select_txout_sql, -1, &select_txout_by_txid_stmt_, NULL);

    rc = sqlite3_prepare_v2(tx_db.ptr(), update_txout_sql, -1, &update_tx_output_stmt_, NULL);

    rc = sqlite3_prepare_v2(tx_db.ptr(), delete_tx_sql, -1, &delete_tx_stmt_, NULL);
    rc = sqlite3_prepare_v2(tx_db.ptr(), delete_txin_sql, -1, &delete_tx_input_stmt_, NULL);
    rc = sqlite3_prepare_v2(tx_db.ptr(), delete_txout_sql, -1, &delete_tx_output_stmt_, NULL);

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

    // db.exec(sql, [](int res, std::string const& error_msg){
    //     if (res != SQLITE_OK) {
    //         fprintf(stderr, "Create SQL error: %s\n", error_msg.c_str());
    //     } else {
    //         fprintf(stdout, "Table created successfully\n");
    //     }
    // });

    tx_db.exec("CREATE TABLE transactions( "
            "id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, "
            "hash TEXT NOT NULL UNIQUE, "
            "version INTEGER NOT NULL, "
            "locktime INTEGER NOT NULL, "
            "block_height INTEGER NOT NULL, "
            "position INTEGER NOT NULL);", [](int res, std::string const& error_msg){

        if (res != SQLITE_OK) {
            //TODO: Fer: Log errors
            res = false;
        }
    });
    if (!res) return res;

    tx_db.exec("CREATE TABLE input ("
            "`id` INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, "
            "`transaction_id` INTEGER NOT NULL, "
            "`prev_output_hash` TEXT NOT NULL, "
            "`prev_output_index` INTEGER NOT NULL, "
            "`script` BLOB, "
            "`sequence` INTEGER NOT NULL );", [](int res, std::string const& error_msg){
        if (res != SQLITE_OK) {
            res = false;
        }
    });
    if (!res) return res;

    tx_db.exec("CREATE TABLE output ( "
            "id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, "
            "transaction_id INTEGER NOT NULL, "
            "idx INTEGER NOT NULL, "
            "amount INTEGER NOT NULL, "
            "script BLOB,"
            "spender_height INTEGER NOT NULL );", [](int res, std::string const& error_msg){
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

    //TODO: Fer: Implement this. Is it necessary?
    return true;
}

// Close files.
bool transaction_database::close() {
//    return lookup_file_.close();
    //TODO: Fer: Implement this. Is it necessary?
    return true;
}

// Commit latest inserts.
void transaction_database::synchronize() {
//    lookup_manager_.sync();
    //TODO: Fer: Implement this. Is it necessary?
}

// Flush the memory map to disk.
bool transaction_database::flush() {
//    return lookup_file_.flush();
    //TODO: Fer: Implement this. Is it necessary?
    return true;
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
        
        // std::vector<uint8_t> script_data(script_ptr, script_ptr + script_size);
        data_chunk script_data(script_ptr, script_ptr + script_size);
        
        auto script = chain::script::factory_from_data(script_data, true);

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

    // return std::make_pair(true, res);
    return res;
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
        // std::vector<uint8_t> script_data(script_ptr, script_ptr + script_size);
        data_chunk script_data(script_ptr, script_ptr + script_size);

        auto spender_height = static_cast<uint32_t>(sqlite3_column_int(stmt, 4));
        

        auto script = chain::script::factory_from_data(script_data, true);

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

    // return std::make_pair(true, res);
    return res;
}


transaction_result transaction_database::get(hash_digest const& hash, size_t /*DEBUG_ONLY(fork_height)*/) const {
    // TODO: use lookup_map_ to search a set of transactions in height order,
    // returning the highest that is at or below the specified fork height.
    // Short-circuit the search if fork_height is max_size_t (just get first).
    ////BITCOIN_ASSERT_MSG(fork_height == max_size_t, "not implemented");
//    const auto memory = lookup_map_.find(hash);
//    return transaction_result(memory, hash);


    sqlite3_bind_text(select_tx_by_hash_stmt_, 1, reinterpret_cast<char const*>(hash.data()), sizeof(hash_digest), SQLITE_STATIC);

    int rc = sqlite3_step(select_tx_by_hash_stmt_);
    if (rc == SQLITE_ROW) {
        auto id = sqlite3_column_int64(select_tx_by_hash_stmt_, 0);
        auto version = static_cast<uint32_t>(sqlite3_column_int(select_tx_by_hash_stmt_, 1));
        auto locktime = static_cast<uint32_t>(sqlite3_column_int(select_tx_by_hash_stmt_, 2));
        auto block_height = static_cast<uint32_t>(sqlite3_column_int(select_tx_by_hash_stmt_, 3));
        auto position = static_cast<uint32_t>(sqlite3_column_int(select_tx_by_hash_stmt_, 4));

        // input::list select_inputs(sqlite3* db, sqlite3_stmt* stmt, int64_t tx_id) {
        auto inputs = select_inputs(tx_db.ptr(), select_txin_by_txid_stmt_, id);
        auto outputs = select_outputs(tx_db.ptr(), select_txout_by_txid_stmt_, id);


        // transaction(uint32_t version, uint32_t locktime, ins&& inputs, outs&& outputs);
        // transaction(uint32_t version, uint32_t locktime, const ins& inputs,
        //     const outs& outputs);

        chain::transaction tx(version, locktime, inputs, outputs);
        // tx.from_data(deserial, use_wire_encoding);

        // TODO: add hash param to deserialization to eliminate this construction.
        // return chain::transaction(std::move(tx), hash_digest(hash));
        tx = chain::transaction(std::move(tx), hash_digest(hash));


        sqlite3_reset(select_tx_by_hash_stmt_);
        return transaction_result(true, hash, tx, block_height, position);
    } else {
        sqlite3_reset(select_tx_by_hash_stmt_);
        return transaction_result(false, hash, chain::transaction(), uint32_t(), uint32_t());
    }
}

bool update_tx_output(sqlite3* db, sqlite3_stmt* stmt,
                              int64_t tx_id,
                              uint32_t index,
                              uint32_t spender_height) {

    sqlite3_bind_int(stmt, 1, spender_height);
    sqlite3_bind_int64(stmt, 2, tx_id);
    sqlite3_bind_int(stmt, 3, index);

    auto rc = sqlite3_step(stmt);

    sqlite3_reset(stmt);

    if (rc != SQLITE_DONE) {
        //TODO: hiding error code
        //printf("ERROR inserting data: %s\n", sqlite3_errmsg(db));
        return false;
    }
    return true;
}


using select_tx_result = std::pair<bool, int64_t>;

select_tx_result select_transaction_id_by_hash(sqlite3* db, sqlite3_stmt* stmt,
                                 hash_digest const& hash) {
    sqlite3_bind_text(stmt, 1, reinterpret_cast<char const*>(hash.data()), sizeof(hash_digest), SQLITE_STATIC);

    int rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        auto id = sqlite3_column_int64(stmt, 0);

        sqlite3_reset(stmt);
        return std::make_pair(true, id);
    } else {
        sqlite3_reset(stmt);
        return std::make_pair(false, 0);
    }
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

    // select_tx_result select_transaction_id_by_hash(sqlite3* db, sqlite3_stmt* stmt, hash_digest const& hash)


    auto res = select_transaction_id_by_hash(tx_db.ptr(), select_tx_by_hash_stmt_, point.hash());

    if (res.first) {
        return update_tx_output(tx_db.ptr(), update_tx_output_stmt_,
                                      res.second,
                                      point.index(),
                                      spender_height);
    }

    return false;
}

using insert_result = std::pair<bool, int64_t>;

insert_result insert_transaction(sqlite3* db, sqlite3_stmt* stmt,
    hash_digest const& hash, uint32_t version, uint32_t locktime,
    uint32_t block_height, uint32_t position) {

    sqlite3_bind_text(stmt, 1, reinterpret_cast<char const*>(hash.data()), sizeof(hash_digest), SQLITE_STATIC);
    sqlite3_bind_int(stmt,  2, version);
    sqlite3_bind_int(stmt,  3, locktime);
    sqlite3_bind_int(stmt,  4, block_height);
    sqlite3_bind_int(stmt,  5, position);


    auto rc = sqlite3_step(stmt);

    sqlite3_reset(stmt);

    if (rc != SQLITE_DONE) {
        //TODO: hiding error code
        //printf("ERROR inserting data: %s\n", sqlite3_errmsg(db));
        return std::make_pair(false, 0);
    }
    return std::make_pair(true, sqlite3_last_insert_rowid(db));
}

insert_result insert_transaction(sqlite3* db, sqlite3_stmt* stmt, chain::transaction const& tx, uint32_t block_height, uint32_t position) {
    return insert_transaction(db, stmt, tx.hash(), tx.version(), tx.locktime(), block_height, position);
}

//sql = "INSERT INTO input (transaction_id, prev_output_hash, prev_output_index, script, sequence) VALUES (?1, ?2, ?3, ?4, ?5);";
insert_result insert_tx_input(sqlite3* db, sqlite3_stmt* stmt,
                              int64_t tx_id,
                              hash_digest const& prev_output_hash,
                              uint32_t prev_output_index,
                              // std::vector<uint8_t> const& script,
                              data_chunk const& script,
                              uint32_t sequence) {

    sqlite3_bind_int64(stmt, 1, tx_id);
    sqlite3_bind_text(stmt, 2, reinterpret_cast<char const*>(prev_output_hash.data()), sizeof(hash_digest), SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, prev_output_index);
    sqlite3_bind_blob(stmt,4, script.data(), script.size(), SQLITE_STATIC);
    sqlite3_bind_int(stmt, 5, sequence);

    auto rc = sqlite3_step(stmt);

    sqlite3_reset(stmt);

    if (rc != SQLITE_DONE) {
        //TODO: hiding error code
        //printf("ERROR inserting data: %s\n", sqlite3_errmsg(db));
        return std::make_pair(false, 0);
    }
    return std::make_pair(true, sqlite3_last_insert_rowid(db));
}

//sql = "INSERT INTO output (transaction_id, index, amount, script) VALUES (?1, ?2, ?3, ?4);";
insert_result insert_tx_output(sqlite3* db, sqlite3_stmt* stmt,
                              int64_t tx_id,
                              uint32_t index,
                              uint64_t amount,
                              std::vector<uint8_t> const& script,
                              uint32_t spender_height) {

    sqlite3_bind_int64(stmt, 1, tx_id);
    sqlite3_bind_int(stmt, 2, index);
    sqlite3_bind_int64(stmt, 3, amount);
    sqlite3_bind_blob(stmt, 4, script.data(), script.size(), SQLITE_STATIC);
    sqlite3_bind_int(stmt, 5, spender_height);

    auto rc = sqlite3_step(stmt);

    sqlite3_reset(stmt);

    if (rc != SQLITE_DONE) {
        //TODO: hiding error code
        //printf("ERROR inserting data: %s\n", sqlite3_errmsg(db));
        return std::make_pair(false, 0);
    }
    return std::make_pair(true, sqlite3_last_insert_rowid(db));
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
//    const auto     = static_cast<size_t>(position);
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

    //BEGIN TRANSACTION

    auto res = insert_transaction(tx_db.ptr(), insert_tx_stmt_, tx, height, position);

    if (!res.first) {
        //TODO: Fer: manipulate Error!
    }

    auto txid = res.second;

    for (auto&& input : tx.inputs()) {
        insert_tx_input(tx_db.ptr(), insert_tx_input_stmt_, txid,
                    input.previous_output().hash(),
                    input.previous_output().index(),
                    input.script().to_data(false),                      //TODO: Fer: Verify if false (prefix parameter) is correct!
                    input.sequence());
    }

    uint32_t index = 0;
    for (auto&& output : tx.outputs()) {

        insert_tx_output(tx_db.ptr(), insert_tx_output_stmt_,
                        txid,
                        index,
                        output.value(),
                        output.script().to_data(false),                 //TODO: Fer: Verify if false (prefix parameter) is correct!
                        output::validation::not_spent);
        ++index;
    }

    //COMMIT TRANSACTION / ROLLBACK TRANSACTION
}



bool delete_tx_generic(sqlite3* db, sqlite3_stmt* stmt, int64_t tx_id) {
    sqlite3_bind_int64(stmt, 1, tx_id);
    auto rc = sqlite3_step(stmt);
    sqlite3_reset(stmt);

    if (rc != SQLITE_DONE) {
        //TODO: hiding error code
        //printf("ERROR inserting data: %s\n", sqlite3_errmsg(db));
        return false;
    }
    return true;
}

bool transaction_database::unlink(hash_digest const& hash) {
//    return lookup_map_.unlink(hash);
    //TODO: Fer: implement this!

    auto res = select_transaction_id_by_hash(tx_db.ptr(), select_tx_by_hash_stmt_, hash);

    if (res.first) {
        auto res2 = delete_tx_generic(tx_db.ptr(), delete_tx_stmt_, res.second);
        if (!res2) return false;

        res2 = delete_tx_generic(tx_db.ptr(), delete_tx_input_stmt_, res.second);
        if (!res2) return false;

        res2 = delete_tx_generic(tx_db.ptr(), delete_tx_output_stmt_, res.second);
        return res2;
    }

    return false;


}

} // namespace database
} // namespace libbitcoin
