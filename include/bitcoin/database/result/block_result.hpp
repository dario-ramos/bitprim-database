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
#ifndef LIBBITCOIN_DATABASE_BLOCK_RESULT_HPP
#define LIBBITCOIN_DATABASE_BLOCK_RESULT_HPP

#include <cstdint>
#include <cstddef>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>

namespace libbitcoin {
namespace database {

/// Deferred read block result.
class BCD_API block_result
{
public:
    //block_result(const memory_ptr slab);
    //block_result(const memory_ptr slab, hash_digest&& hash);
    //block_result(const memory_ptr slab, const hash_digest& hash);

    block_result(bool valid, uint32_t id, hash_digest hash, chain::header const& block, std::vector<hash_digest>);

    /// True if this block result is valid (found).
    operator bool() const;

    /// The block header hash (from cache).
    const hash_digest& hash() const;

    /// The block header.
    chain::header header() const;

    /// The height of this block in the chain.
    size_t height() const;

    /// The header.bits of this block.
    uint32_t bits() const;

    /// The header.timestamp of this block.
    uint32_t timestamp() const;

    /// The header.version of this block.
    uint32_t version() const;

    /// The number of transactions in this block.
    size_t transaction_count() const;

    /// A transaction hash where index < transaction_count.
    hash_digest transaction_hash(size_t index) const;

private:
    bool valid_;
    const hash_digest hash_;
    const uint32_t id_;
    chain::header block_header_;
    std::vector<hash_digest> tx_hashes_;

};

} // namespace database
} // namespace libbitcoin

#endif
