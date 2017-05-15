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
#include <bitcoin/database/databases/transaction_unconfirmed_database_mem.hpp>

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

const size_t transaction_unconfirmed_database_mem::unconfirmed = max_uint32;

// Transactions uses a hash table index, O(1).
transaction_unconfirmed_database_mem::transaction_unconfirmed_database_mem(size_t buckets, size_t expansion, mutex_ptr mutex)
{}

transaction_unconfirmed_database_mem::~transaction_unconfirmed_database_mem() {
    close();
}

// Create.
// ----------------------------------------------------------------------------

// Initialize files and start.
bool transaction_unconfirmed_database_mem::create() {
    return true;
}

// Startup and shutdown.
// ----------------------------------------------------------------------------

bool transaction_unconfirmed_database_mem::open() {
    return true;
}

bool transaction_unconfirmed_database_mem::close() {
    return true;
}

// Commit latest inserts.
void transaction_unconfirmed_database_mem::synchronize() {}

// Flush the memory map to disk.
bool transaction_unconfirmed_database_mem::flush() const {
    return true;
}

// Queries.
// ----------------------------------------------------------------------------

//memory_ptr transaction_unconfirmed_database_mem::find(const hash_digest& hash) const
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



//transaction_result transaction_unconfirmed_database_mem::get(hash_digest const& hash) const
boost::optional<chain::transaction> transaction_unconfirmed_database_mem::get(hash_digest const& hash) const  {
    auto it = data_.find(hash);
    if (it == data_.end())  boost::optional<chain::transaction>();
    return boost::optional<chain::transaction>(*it);
}

void transaction_unconfirmed_database_mem::store(chain::transaction const& tx) {
    auto const hash = tx.hash();
    data_.emplace(hash, tx);
}

// bool transaction_unconfirmed_database_mem::spend(const output_point& point, size_t spender_height)
// bool transaction_unconfirmed_database_mem::unspend(const output_point& point)
// bool transaction_unconfirmed_database_mem::confirm(const hash_digest& hash, size_t height, size_t position)
// bool transaction_unconfirmed_database_mem::unconfirm(const hash_digest& hash)

bool transaction_unconfirmed_database_mem::unlink(hash_digest const& hash) {
    return data_.erase(hash) == 1;
}

bool transaction_unconfirmed_database_mem::unlink_if_exists(hash_digest const& hash) {
    return data_.erase(hash) == 1;
}


} // namespace database
} // namespace libbitcoin
