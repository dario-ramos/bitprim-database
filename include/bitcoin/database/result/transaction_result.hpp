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
#ifndef LIBBITCOIN_DATABASE_TRANSACTION_RESULT_HPP
#define LIBBITCOIN_DATABASE_TRANSACTION_RESULT_HPP

#include <cstddef>
#include <cstdint>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>

namespace libbitcoin {
namespace database {
    
/// Deferred read transaction result.
class BCD_API transaction_result
{
public:
    transaction_result(const memory_ptr slab);
    transaction_result(const memory_ptr slab, hash_digest&& hash);
    transaction_result(const memory_ptr slab, const hash_digest& hash);

    /// True if this transaction result is valid (found).
    operator bool() const;

    /// The transaction hash (from cache).
    const hash_digest& hash() const;

    /// The height of the block which includes the transaction.
    size_t height() const;

    /// The ordinal position of the transaction within its block.
    size_t position() const;

    /// True if all transaction outputs are spent at or below fork_height.
    bool is_spent(size_t fork_height) const;

    /// The output at the specified index within this transaction.
    chain::output output(uint32_t index) const;

    /// The transaction.
    chain::transaction transaction() const;

private:
    // /*const*/ memory_ptr slab_;
    const memory_ptr slab_;
    const hash_digest hash_;
};

} // namespace database
} // namespace libbitcoin

#endif
