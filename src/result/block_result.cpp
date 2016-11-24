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
#include <bitcoin/database/result/block_result.hpp>

#include <cstdint>
#include <cstddef>
#include <utility>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/memory/memory.hpp>

namespace libbitcoin {
namespace database {

using namespace bc::chain;

static constexpr size_t version_size = sizeof(uint32_t);
static constexpr size_t previous_size = hash_size;
static constexpr size_t merkle_size = hash_size;
static constexpr size_t time_size = sizeof(uint32_t);
static constexpr size_t bits_size = sizeof(uint32_t);
static constexpr size_t nonce_size = sizeof(uint32_t);
static constexpr size_t height_size = sizeof(uint32_t);

static constexpr auto version_offset = size_t(0);
static constexpr auto time_offset = version_size + previous_size + merkle_size;
static constexpr auto bits_offset = time_offset + time_size;
static constexpr auto height_offset = bits_offset + bits_size + nonce_size;
static constexpr auto count_offset = height_offset + height_size;

block_result::block_result(bool valid, uint32_t id, hash_digest hash, chain::header const& block, std::vector<hash_digest> tx_hashes)
  : valid_(valid), id_(id), hash_(hash),block_header_(block),tx_hashes_(tx_hashes)
{
}

block_result::operator bool() const
{
  return valid_;
}

const hash_digest& block_result::hash() const
{
    return hash_;
}

chain::header block_result::header() const
{
  return block_header_;
}

size_t block_result::height() const
{
    return id_;
}

uint32_t block_result::bits() const
{
    return block_header_.bits();
}

uint32_t block_result::timestamp() const
{
    return block_header_.timestamp();
}

uint32_t block_result::version() const
{
    return block_header_.version();
}

size_t block_result::transaction_count() const
{
  return tx_hashes_.size();
}

hash_digest block_result::transaction_hash(size_t index) const
{
    if(index<=tx_hashes_.size())
    {
      return tx_hashes_.at(index);
    } else
    { 
      hash_digest hash;
      return hash;
    } 
}

} // namespace database
} // namespace libbitcoin
