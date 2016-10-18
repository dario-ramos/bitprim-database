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
#ifndef LIBBITCOIN_DATABASE_SLAB_HASH_TABLE_IPP
#define LIBBITCOIN_DATABASE_SLAB_HASH_TABLE_IPP

#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include "../impl/remainder.ipp"
#include "../impl/slab_row.ipp"

namespace libbitcoin {
namespace database {

template <typename KeyType>
slab_hash_table<KeyType>::slab_hash_table(slab_hash_table_header& header,
    slab_manager& manager)
  : header_(header), manager_(manager)
{
}

// This is not limited to storing unique key values. If duplicate keyed values
// are store then retrieval and unlinking will fail as these multiples cannot
// be differentiated. Therefore the database is not currently able to support
// multiple transactions with the same hash, as required by BIP30.
template <typename KeyType>
file_offset slab_hash_table<KeyType>::store(const KeyType& key,
    write_function write, const size_t value_size)
{
    // Store current bucket value.
    const auto old_begin = read_bucket_value(key);
    slab_row<KeyType> item(manager_, 0);
    const auto new_begin = item.create(key, value_size, old_begin);
    write(item.data());

    // Link record to header.
    link(key, new_begin);

    // Return position,
    return new_begin + item.value_begin;
}

// This is limited to returning the first of multiple matching key values.
template <typename KeyType>
const memory_ptr slab_hash_table<KeyType>::find(const KeyType& key) const
{
    // Find start item...
    auto current = read_bucket_value(key);

    // Iterate through list...
    while (current != header_.empty)
    {
        const slab_row<KeyType> item(manager_, current);

        // Found.
        if (item.compare(key))
            return item.data();

        const auto previous = current;
        current = item.next_position();

        // This may otherwise produce an infinite loop here.
        // It indicates that a write operation has interceded.
        // So we must return gracefully vs. looping forever.
        if (previous == current)
            return nullptr;
    }

    return nullptr;
}

// This is limited to unlinking the first of multiple matching key values.
template <typename KeyType>
bool slab_hash_table<KeyType>::unlink(const KeyType& key)
{
    // Find start item...
    const auto begin = read_bucket_value(key);
    const slab_row<KeyType> begin_item(manager_, begin);

    // If start item has the key then unlink from buckets.
    if (begin_item.compare(key))
    {
        link(key, begin_item.next_position());
        return true;
    }

    // Continue on...
    auto previous = begin;
    auto current = begin_item.next_position();

    // Iterate through list...
    while (current != header_.empty)
    {
        const slab_row<KeyType> item(manager_, current);

        // Found, unlink current item from previous.
        if (item.compare(key))
        {
            release(item, previous);
            return true;
        }

        previous = current;
        current = item.next_position();

        // This may otherwise produce an infinite loop here.
        // It indicates that a write operation has interceded.
        // So we must return gracefully vs. looping forever.
        if (previous == current)
            return false;
    }

    return false;
}

template <typename KeyType>
array_index slab_hash_table<KeyType>::bucket_index(const KeyType& key) const
{
    const auto bucket = remainder(key, header_.size());
    BITCOIN_ASSERT(bucket < header_.size());
    return bucket;
}

template <typename KeyType>
file_offset slab_hash_table<KeyType>::read_bucket_value(
    const KeyType& key) const
{
    const auto value = header_.read(bucket_index(key));
    static_assert(sizeof(value) == sizeof(file_offset), "Invalid size");
    return value;
}

template <typename KeyType>
void slab_hash_table<KeyType>::link(const KeyType& key,
    const file_offset begin)
{
    header_.write(bucket_index(key), begin);
}

template <typename KeyType>
template <typename ListItem>
void slab_hash_table<KeyType>::release(const ListItem& item,
    const file_offset previous)
{
    ListItem previous_item(manager_, previous);
    previous_item.write_next_position(item.next_position());
}


// -------------- Fernando (remove) -------------------------
template <typename KeyType>
file_offset slab_hash_table<KeyType>::read_bucket_value_by_index(size_t bucket) const {
    const auto value = header_.read(bucket);
    static_assert(sizeof(value) == sizeof(file_offset), "Invalid size");
    return value;
}

template <typename KeyType>
file_offset slab_hash_table<KeyType>::read_first_bucket_value() const {
    return read_bucket_value_by_index(0);
}

template <typename KeyType>
std::tuple<size_t, file_offset, memory_ptr> slab_hash_table<KeyType>::get_first_item() const {

    auto current = read_first_bucket_value();

    if (current != header_.empty) {
        const slab_row<KeyType> item(manager_, current);
        // return {0, current, item.data()};
        return std::make_tuple(0, current, item.data());
    }

    // return {0, current, nullptr};
    return std::make_tuple(0, current, nullptr);
}

template <typename KeyType>
std::tuple<size_t, file_offset, memory_ptr> slab_hash_table<KeyType>::get_next_item(size_t bucket, file_offset current) const {

    // auto current = read_first_bucket_value();
    auto item = slab_row<KeyType>(manager_, current);
    const auto previous = current;
    current = item.next_position();

    if (current == header_.empty) {
        ++bucket;
        current = read_bucket_value_by_index(bucket);
        item = slab_row<KeyType>(manager_, current);
    }

    if (current != header_.empty) {
        // return {bucket, current, item.data()};
        return std::make_tuple(bucket, current, item.data());

    }

    // return {bucket, current, nullptr};
    return std::make_tuple(bucket, current, nullptr);
}


} // namespace database
} // namespace libbitcoin

#endif
