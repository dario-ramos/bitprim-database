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
#include <bitcoin/database/databases/block_database.hpp>

#include <cstdint>
#include <cstddef>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/result/block_result.hpp>

namespace libbitcoin {
namespace database {

using namespace bc::chain;

// Valid file offsets should never be zero.
const file_offset block_database::empty = 0;

static constexpr auto index_header_size = 0u;
static constexpr auto index_record_size = sizeof(file_offset);

// Record format:
//  [ header:80 ]
//  [ height:4 ]
//  TODO: [ checksum:4 ] (store all zeros if not computed).
//  [ tx_count:1-2 ]
//  [ [ tx_hash:32 ]... ]

// See block result.cpp.
static constexpr auto height_offset = 80u;

// Blocks uses a hash table and an array index, both O(1).
block_database::block_database(const path& map_filename,
    const path& index_filename, size_t buckets, size_t expansion,
    mutex_ptr mutex)
  : initial_map_file_size_(slab_hash_table_header_size(buckets) +
        minimum_slabs_size),

    lookup_file_(map_filename, mutex, expansion),
    lookup_header_(lookup_file_, buckets),
    lookup_manager_(lookup_file_, slab_hash_table_header_size(buckets)),
    lookup_map_(lookup_header_, lookup_manager_),

    index_file_(index_filename, mutex, expansion),
    index_manager_(index_file_, index_header_size, index_record_size)
{
}

block_database::~block_database()
{
    close();
}

// Create.
// ----------------------------------------------------------------------------

// Initialize files and open.
bool block_database::create()
{
    // Resize and create require an opened file.
    if (!lookup_file_.open() ||
        !index_file_.open())
        return false;

    // These will throw if insufficient disk space.
    lookup_file_.resize(initial_map_file_size_);
    index_file_.resize(minimum_records_size);

    if (!lookup_header_.create() ||
        !lookup_manager_.create() ||
        !index_manager_.create())
        return false;

    // Should not call open after create, already started.
    return
        lookup_header_.start() &&
        lookup_manager_.start() &&
        index_manager_.start();
}

// Startup and shutdown.
// ----------------------------------------------------------------------------

// Start files and primitives.
bool block_database::open()
{
    return
        lookup_file_.open() &&
        index_file_.open() &&
        lookup_header_.start() &&
        lookup_manager_.start() &&
        index_manager_.start();
}

// Close files.
bool block_database::close()
{
    return
        lookup_file_.close() &&
        index_file_.close();
}

// Update logical file sizes.
void block_database::synchronize()
{
    lookup_manager_.sync();
    index_manager_.sync();
}

// Flush the memory maps to disk.
bool block_database::flush() const
{
    return
        lookup_file_.flush() &&
        index_file_.flush();
}

// Queries.
// ----------------------------------------------------------------------------

bool block_database::exists(size_t height) const
{
    return height < index_manager_.count() && read_position(height) != empty;
}

block_result block_database::get(size_t height) const
{
    if (height >= index_manager_.count())
        return{};

    // No need to read height here since it's the key.
    const auto position = read_position(height);
    const auto slab = lookup_manager_.get(position);

    //*************************************************************************
    // HACK: back up into the slab to obtain the key (optimization).
    static const auto prefix_size = slab_row<hash_digest>::prefix_size;
    const auto memory = REMAP_ADDRESS(slab);
    auto deserial = make_unsafe_deserializer(memory - prefix_size);
    //*************************************************************************

    return block_result(slab, deserial.read_hash(), height);
}

block_result block_database::get(const hash_digest& hash) const
{
    const auto slab = lookup_map_.find(hash);

    if (slab)
    {
        ///////////////////////////////////////////////////////////////////////
        metadata_mutex_.lock_shared();
        const auto height_start = REMAP_ADDRESS(slab) + height_offset;
        const auto height = from_little_endian_unsafe<uint32_t>(height_start);
        metadata_mutex_.unlock_shared();
        ///////////////////////////////////////////////////////////////////////

        return block_result(slab, hash, height);
    }

    return{};
}

//WARNING!! : This is public interface, but apparently it is not used in Blockchain
void block_database::store(const block& block, size_t height)
{
    BITCOIN_ASSERT(height <= max_uint32);
    const auto height32 = static_cast<uint32_t>(height);
    const auto tx_count = block.transactions().size();

    // Write block data.
    const auto write = [&](serializer<uint8_t*>& serial)
    {
        // WRITE THE BLOCK HEADER AND TX HASHES
        block.header().to_data(serial);

        ///////////////////////////////////////////////////////////////////////
        // Critical Section
        metadata_mutex_.lock();
        serial.write_4_bytes_little_endian(height32);
        serial.write_size_little_endian(tx_count);
        metadata_mutex_.unlock();
        ///////////////////////////////////////////////////////////////////////

        // WRITE THE TX HASHES
        for (const auto& tx: block.transactions())
            serial.write_hash(tx.hash());
    };

    const auto key = block.header().hash();
    const auto size = header::satoshi_fixed_size() + sizeof(height32) +
        message::variable_uint_size(tx_count) + (tx_count * hash_size);

    const auto position = lookup_map_.store(key, write, size);

    // Write position to index.
    write_position(position, height32);
}

bool block_database::gaps(heights& out_gaps) const
{
    const auto count = index_manager_.count();

    for (size_t height = 0; height < count; ++height)
        if (read_position(height) == empty)
            out_gaps.push_back(height);

    return true;
}

bool block_database::unlink(size_t from_height)
{
    if (index_manager_.count() > from_height)
    {
        index_manager_.set_count(from_height);
        return true;
    }

    return false;
}

// This is necessary for parallel import, as gaps are created.
void block_database::zeroize(array_index first, array_index count)
{
    for (auto index = first; index < (first + count); ++index)
    {
        const auto slab = index_manager_.get(index);
        auto serial = make_unsafe_serializer(REMAP_ADDRESS(slab));
        serial.write_8_bytes_little_endian(empty);
    }
}

void block_database::write_position(file_offset position, array_index height)
{
    BITCOIN_ASSERT(height < max_uint32);
    const auto new_count = height + 1;

    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    mutex_.lock_upgrade();

    // Guard index_manager to prevent interim count increase.
    const auto initial_count = index_manager_.count();

    //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    mutex_.unlock_upgrade_and_lock();

    // Guard write to prevent overwriting preceding height write.
    if (new_count > initial_count)
    {
        const auto create_count = new_count - initial_count;
        index_manager_.new_records(create_count);
        zeroize(initial_count, create_count - 1);
    }

    // Guard write to prevent subsequent zeroize from erasing.
    const auto slab = index_manager_.get(height);
    auto serial = make_unsafe_serializer(REMAP_ADDRESS(slab));
    serial.write_8_bytes_little_endian(position);

    mutex_.unlock();
    ///////////////////////////////////////////////////////////////////////////
}

file_offset block_database::read_position(array_index height) const
{
    const auto slab = index_manager_.get(height);
    return from_little_endian_unsafe<file_offset>(REMAP_ADDRESS(slab));
}

// The index of the highest existing block, independent of gaps.
bool block_database::top(size_t& out_height) const
{
    const auto count = index_manager_.count();

    // Guard against no genesis block.
    if (count == 0)
        return false;

    out_height = count - 1;
    return true;
}

} // namespace database
} // namespace libbitcoin
