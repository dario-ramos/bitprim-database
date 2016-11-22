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
#ifndef LIBBITCOIN_DATABASE_TRANSACTION_DATABASE_HPP
#define LIBBITCOIN_DATABASE_TRANSACTION_DATABASE_HPP

#include <cstddef>
#include <memory>
#include <boost/filesystem.hpp>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/result/transaction_result.hpp>
#include <bitcoin/database/primitives/db_connection.hpp>

namespace libbitcoin {
namespace database {

/// This enables lookups of transactions by hash.
/// An alternative and faster method is lookup from a unique index
/// that is assigned upon storage.
/// This is so we can quickly reconstruct blocks given a list of tx indexes
/// belonging to that block. These are stored with the block.
class BCD_API transaction_database {
public:
    using path = boost::filesystem::path;
//    typedef std::shared_ptr<shared_mutex> mutex_ptr;

    /// Construct the database.
    transaction_database(path const& filename);

    /// Close the database (all threads must first be stopped).
    ~transaction_database();

    /// Initialize a new transaction database.
    bool create();

    /// Call before using the database.
    bool open();

    /// Call to unload the memory map.
    bool close();

    /// Fetch transaction by its hash.
    transaction_result get(hash_digest const& hash) const;

    /// Fetch transaction by its hash, at or below the specified block height.
    transaction_result get(hash_digest const& hash, size_t fork_height) const;

    /// Store a transaction in the database.
    void store(size_t height, size_t position, chain::transaction const& tx);

    /// Update the spender height of the output in the tx store.
    bool update(chain::output_point const& point, size_t spender_height);

    /// Delete a transaction from database.
    bool unlink(hash_digest const& hash);

    /// Commit latest inserts.
    void synchronize();

    /// Flush the memory map to disk.
    bool flush();

private:
    bitprim::db_connection tx_db;

    // Prepared Statements
    sqlite3_stmt* insert_tx_stmt_;
    sqlite3_stmt* insert_tx_input_stmt_;
    sqlite3_stmt* insert_tx_output_stmt_;

    sqlite3_stmt* select_tx_by_hash_stmt_;
    sqlite3_stmt* select_txin_by_txid_stmt_;
    sqlite3_stmt* select_txout_by_txid_stmt_;

    sqlite3_stmt* update_tx_output_stmt_;

    sqlite3_stmt* delete_tx_stmt_;
    sqlite3_stmt* delete_tx_input_stmt_;
    sqlite3_stmt* delete_tx_output_stmt_;

};

} // namespace database
} // namespace libbitcoin

#endif
