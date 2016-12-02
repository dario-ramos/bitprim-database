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
#include <bitcoin/database/databases/spend_database.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/memory/memory.hpp>

namespace libbitcoin {
namespace database {

using namespace bc::chain;

static constexpr auto value_size = std::tuple_size<point>::value;

static constexpr char insert_spend_sql[] = "INSERT INTO spend (output_hash, output_index, input_hash, input_index) VALUES (?1, ?2, ?3, ?4);";
static constexpr char select_spend_sql[] = "SELECT input_hash, input_index FROM spend WHERE output_hash = ?1 AND output_index = ?2;";

// Spends use a hash table index, O(1).
spend_database::spend_database(path const& filename)
  : spend_db(filename.c_str())
{}

spend_database::~spend_database() {
    close();
}


bool spend_database::prepare_statements() {
    std::cout << "bool spend_database::prepare_statements()\n";
    int rc;

    rc = sqlite3_prepare_v2(spend_db.ptr(), insert_spend_sql, -1, &insert_spend_stmt_, NULL);
    std::cout << "rc: " << rc << '\n';
//    std::cout << "rc: " << (rc == SQLITE_OK) << '\n';
    printf("ERROR: %s\n", sqlite3_errmsg(spend_db.ptr()));


    rc = sqlite3_prepare_v2(spend_db.ptr(), select_spend_sql, -1, &select_spend_stmt_, NULL);
    std::cout << "rc: " << rc << '\n';
//    std::cout << "rc: " << (rc == SQLITE_OK) << '\n';
    printf("ERROR: %s\n", sqlite3_errmsg(spend_db.ptr()));


    //TODO: Fer: check for errors

    std::cout << "bool transaction_database::prepare_statements() -- END\n";

    return true;

}


// Create.
// ----------------------------------------------------------------------------

// Initialize files and start.
bool spend_database::create()  {

    std::cout << "bool spend_database::create()\n";

    bool res = true;

    spend_db.exec("CREATE TABLE spend( "
                       "id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, "
                       "output_hash TEXT NOT NULL UNIQUE, "
                       "output_index INTEGER NOT NULL, "
                       "input_hash TEXT NOT NULL UNIQUE, "
                       "input_index INTEGER NOT NULL);", [&res](int reslocal, std::string const& error_msg){

        if (reslocal != SQLITE_OK) {
            //TODO: Fer: Log errors
            res = false;
        }
    });
    if (!res) return res;
}

// Startup and shutdown.
// ----------------------------------------------------------------------------

bool spend_database::open() {
    return prepare_statements();
}

bool spend_database::close() {
    //TODO: Fer: Implement this. Is it necessary?
    return true;

}

// Commit latest inserts.
void spend_database::synchronize() {
    //TODO: Fer: Implement this. Is it necessary?
}

// Flush the memory map to disk.
bool spend_database::flush() {
    //TODO: Fer: Implement this. Is it necessary?
    //TODO: Fer: Check if I have to use some kind of Flush on SQLite
    return true;
}

// Queries.
// ----------------------------------------------------------------------------

input_point spend_database::get(output_point const& outpoint) const {
    //std::cout << "spend spend_database::get(const output_point& outpoint) const\n";
    input_point point;

    sqlite3_reset(select_spend_stmt_);
    sqlite3_bind_text(select_spend_stmt_, 1, reinterpret_cast<char const*>(outpoint.hash().data()), sizeof(hash_digest), SQLITE_STATIC);
    sqlite3_bind_int(insert_spend_stmt_,  2, outpoint.index());

    // std::cout << "transaction_result transaction_database::get(hash_digest const& hash, size_t) const - 2 - " << std::this_thread::get_id() << "\n";

    int rc = sqlite3_step(select_spend_stmt_);
    if (rc == SQLITE_ROW) {

        // std::cout << "transaction_result transaction_database::get(hash_digest const& hash, size_t) const - 3 - " << std::this_thread::get_id() << "\n";


        hash_digest input_hash;
        memcpy(input_hash.data(), sqlite3_column_text(select_spend_stmt_, 0), sizeof(input_hash));
        auto input_index = static_cast<uint32_t>(sqlite3_column_int(select_spend_stmt_, 1));

        point.set_hash(input_hash);
        point.set_index(input_index);


    } else if (rc == SQLITE_DONE) {
        // std::cout << "transaction_result transaction_database::get(hash_digest const& hash, size_t) const -- END no data found\n";
        // std::cout << "hash: " << encode_hash(hash) << std::endl;
    } else {
        std::cout << "transaction_result transaction_database::get(hash_digest const& hash, size_t) const -- END with Error\n";
        std::cout << "rc: " << rc << '\n';
//        std::cout << "rc: " << (rc == SQLITE_OK) << '\n';
        printf("ERROR in query: %s\n", sqlite3_errmsg(spend_db.ptr()));
    }

    return point;
}

void spend_database::store(chain::output_point const& outpoint, chain::input_point const& spend) {
    //std::cout << "void spend_database::store(const chain::output_point& outpoint, const chain::input_point& spend)\n";

    sqlite3_reset(insert_spend_stmt_);
    sqlite3_bind_text(insert_spend_stmt_, 1, reinterpret_cast<char const*>(outpoint.hash().data()), sizeof(hash_digest), SQLITE_STATIC);
    sqlite3_bind_int(insert_spend_stmt_,  2, outpoint.index());
    sqlite3_bind_text(insert_spend_stmt_, 3, reinterpret_cast<char const*>(spend.hash().data()), sizeof(hash_digest), SQLITE_STATIC);
    sqlite3_bind_int(insert_spend_stmt_,  4, spend.index());

    auto rc = sqlite3_step(insert_spend_stmt_);

    if (rc != SQLITE_DONE) {

        std::cout << "insert_result insert_transaction(sqlite3* db, sqlite3_stmt* stmt,...) -- END with Error\n";
        std::cout << "rc: " << rc << std::endl;

//        std::cout << "hash: " << encode_hash(hash) << std::endl;
//        std::cout << "version: " << version << std::endl;
//        std::cout << "locktime: " << locktime << std::endl;
//        std::cout << "block_height: " << block_height << std::endl;
//        std::cout << "position: " << position << std::endl;

        //TODO: hiding error code
        printf("ERROR: %s\n", sqlite3_errmsg(spend_db.ptr()));
        return; //false
    }

    return; //true
    //sqlite3_last_insert_rowid(db)
}

bool spend_database::unlink(output_point const& outpoint) {
    //std::cout << "bool spend_database::unlink(const output_point& outpoint)\n";

    //TODO: Fer
    return true;
}

//spend_statinfo spend_database::statinfo() const  {
//    return {0, 0};
//}

} // namespace database
} // namespace libbitcoin
