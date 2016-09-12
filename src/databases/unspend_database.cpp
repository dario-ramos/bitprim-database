/**
 * Copyright (c) 2016 Bitprim developers (see AUTHORS)
 *
 * This file is part of Bitprim.
 *
 * Bitprim is free software: you can redistribute it and/or modify
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
#include <bitcoin/database/databases/unspend_database.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <boost/filesystem.hpp>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/memory/memory.hpp>

namespace libbitcoin {
namespace database {

using namespace boost::filesystem;
using namespace bc::chain;

BC_CONSTEXPR size_t number_buckets = 45000000; //228110589;
BC_CONSTEXPR size_t header_size = record_hash_table_header_size(number_buckets);
BC_CONSTEXPR size_t initial_map_file_size = header_size + minimum_records_size;

// BC_CONSTEXPR size_t value_size = std::tuple_size<chain::point>::value;
// BC_CONSTEXPR size_t record_size = hash_table_record_size<chain::point>(value_size);
BC_CONSTEXPR size_t record_size = hash_table_set_record_size<chain::point>();

unspend_database::unspend_database(path const& filename,
    std::shared_ptr<shared_mutex> mutex)
  : lookup_file_(filename, mutex), 
    lookup_header_(lookup_file_, number_buckets),
    lookup_manager_(lookup_file_, header_size, record_size),
    lookup_map_(lookup_header_, lookup_manager_)
{
}

// Close does not call stop because there is no way to detect thread join.
unspend_database::~unspend_database()
{
    close();
}

// Create.
// ----------------------------------------------------------------------------

// Initialize files and start.
bool unspend_database::create()
{
    // Resize and create require a started file.
    if (!lookup_file_.start())
        return false;

    // This will throw if insufficient disk space.
    lookup_file_.resize(initial_map_file_size);

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

bool unspend_database::start()
{
    return
        lookup_file_.start() &&
        lookup_header_.start() &&
        lookup_manager_.start();
}

bool unspend_database::stop()
{
    return lookup_file_.stop();
}

bool unspend_database::close()
{
    return lookup_file_.close();
}

// ----------------------------------------------------------------------------

bool unspend_database::contains(output_point const& outpoint) const
{
    return lookup_map_.contains(outpoint);
}

void unspend_database::store(chain::output_point const& outpoint)
{
    lookup_map_.store(outpoint);
}

void unspend_database::remove(output_point const& outpoint)
{
    DEBUG_ONLY(bool success =) lookup_map_.unlink(outpoint);
    BITCOIN_ASSERT(success);
}

void unspend_database::sync()
{
    lookup_manager_.sync();
}

unspend_statinfo unspend_database::statinfo() const
{
    return
    {
        lookup_header_.size(),
        lookup_manager_.count()
    };
}

} // namespace database
} // namespace libbitcoin
