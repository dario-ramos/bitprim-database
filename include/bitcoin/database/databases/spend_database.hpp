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
#ifndef LIBBITCOIN_DATABASE_SPEND_DATABASE_HPP
#define LIBBITCOIN_DATABASE_SPEND_DATABASE_HPP

#include <cstddef>
#include <memory>
#include <boost/filesystem.hpp>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/define.hpp>

#include <bitcoin/database/primitives/db_connection.hpp>


namespace libbitcoin {
namespace database {

//struct BCD_API spend_statinfo
//{
//    /// Number of buckets used in the hashtable.
//    /// load factor = rows / buckets
//    const size_t buckets;
//
//    /// Total number of spend rows.
//    const size_t rows;
//};

/// This enables you to lookup the spend of an output point, returning
/// the input point. It is a simple map.
class BCD_API spend_database
{
public:
    typedef boost::filesystem::path path;

    /// Construct the database.
    spend_database(path const& filename);

    /// Close the database (all threads must first be stopped).
    ~spend_database();

    /// Initialize a new spend database.
    bool create();

    /// Call before using the database.
    bool open();

    /// Call to unload the memory map.
    bool close();

    /// Get inpoint that spent the given outpoint.
    chain::input_point get(chain::output_point const& outpoint) const;

    /// Store a spend in the database.
    void store(chain::output_point const& outpoint, chain::input_point const& spend);

    /// Delete outpoint spend item from database.
    bool unlink(chain::output_point const& outpoint);

    /// Commit latest inserts.
    void synchronize();

    /// Flush the memory map to disk.
    bool flush();

    /// Return statistical info about the database.
//    spend_statinfo statinfo() const;

private:
    bool prepare_statements();

    bitprim::db_connection spend_db;

    // Prepared Statements
    sqlite3_stmt* insert_spend_stmt_;
    sqlite3_stmt* select_spend_stmt_;


};

} // namespace database
} // namespace libbitcoin

#endif
