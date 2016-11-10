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
#ifndef LIBBITCOIN_DATABASE_STEALTH_DATABASE_HPP
#define LIBBITCOIN_DATABASE_STEALTH_DATABASE_HPP

#include <cstdint>
#include <memory>
#include <boost/filesystem.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/memory/memory_map.hpp>
#include <bitcoin/database/primitives/record_manager.hpp>

namespace libbitcoin {
namespace database {

class BCD_API stealth_database
{
public:
    typedef chain::stealth_compact::list list;
    typedef std::function<void(memory_ptr)> write_function;
    typedef boost::filesystem::path path;
    typedef std::shared_ptr<shared_mutex> mutex_ptr;

    /// Construct the database.
    stealth_database(const path& rows_filename, size_t expansion,
        mutex_ptr mutex=nullptr);

    /// Close the database (all threads must first be stopped).
    ~stealth_database();

    /// Initialize a new stealth database.
    bool create();

    /// Call before using the database.
    bool open();

    /// Call to unload the memory map.
    bool close();

    /// Linearly scan all entries, discarding those after from_height.
    list scan(const binary& filter, size_t from_height) const;

    /// Add a stealth row to the database.
    void store(uint32_t prefix, uint32_t height,
        const chain::stealth_compact& row);

    /// Delete stealth row (not implemented.
    bool unlink();

    /// Commit latest inserts.
    void synchronize();

    /// Flush the memory map to disk.
    bool flush();

private:
    void write_index();
    array_index read_index(size_t from_height) const;

    // Row entries containing stealth tx data.
    memory_map rows_file_;
    record_manager rows_manager_;
};

} // namespace database
} // namespace libbitcoin

#endif
