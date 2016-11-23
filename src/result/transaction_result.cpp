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
#include <bitcoin/database/result/transaction_result.hpp>

#include <cstddef>
#include <cstdint>
#include <utility>
#include <bitcoin/bitcoin.hpp>
// #include <bitcoin/database/memory/memory.hpp>

namespace libbitcoin {
namespace database {

using namespace bc::chain;

static const auto use_wire_encoding = false;

transaction_result::transaction_result(bool valid, hash_digest hash, chain::transaction const& tx, uint32_t block_height, uint32_t position)
 : valid_(valid), hash_(hash), tx_(tx), block_height_(block_height), position_(position)
{}

transaction_result::operator bool() const {
    // return slab_ != nullptr;
    return valid_;
}

hash_digest const& transaction_result::hash() const {
    return hash_;
}

 size_t transaction_result::height() const {
     // BITCOIN_ASSERT(slab_);
     // const auto memory = REMAP_ADDRESS(slab_);
     // return from_little_endian_unsafe<uint32_t>(memory);
     return block_height_;
 }

 size_t transaction_result::position() const {
     // BITCOIN_ASSERT(slab_);
     // const auto memory = REMAP_ADDRESS(slab_);
     // return from_little_endian_unsafe<uint32_t>(memory + height_size);
     return position_;
 }

bool transaction_result::is_spent(size_t fork_height) const {
    // static const auto not_spent = output::validation::not_spent;

    // BITCOIN_ASSERT(slab_);
    // const auto memory = REMAP_ADDRESS(slab_);
    // const auto tx_start = memory + height_size + position_size;
    // auto deserial = make_unsafe_deserializer(tx_start);
    // deserial.skip(version_size + locktime_size);
    // const auto outputs = deserial.read_size_little_endian();
    // BITCOIN_ASSERT(deserial);

    // // Search all outputs for an unspent indication.
    // for (uint32_t output = 0; output < outputs; ++output)
    // {
    //     const auto spender_height = deserial.read_4_bytes_little_endian();
    //     BITCOIN_ASSERT(deserial);

    //     // A spend from above the fork height is not considered a spend.
    //     // There cannot also be a spend below the fork height, so it's unspent.
    //     if (spender_height == not_spent || spender_height > fork_height)
    //         return false;

    //     deserial.skip(value_size);
    //     deserial.skip(deserial.read_size_little_endian());
    //     BITCOIN_ASSERT(deserial);
    // }

    // return true;

    //TODO: Fer: change this to auto& when the return type of transaction_result::transaction() be corrected!
    auto tx = transaction();

    auto f = std::find_if(std::begin(tx.outputs()), std::end(tx.outputs()), 
        [fork_height](chain::output const& x){
            // A spend from above the fork height is not considered a spend.
            // There cannot also be a spend below the fork height, so it's unspent.
            return (x.validation.spender_height == output::validation::not_spent || x.validation.spender_height > fork_height);
    });

    return f == std::end(tx.outputs());
}

// If index is out of range returns default/invalid output (.value not_found).
chain::output transaction_result::output(uint32_t index) const {
    // BITCOIN_ASSERT(slab_);
    // const auto memory = REMAP_ADDRESS(slab_);
    // const auto tx_start = memory + height_size + position_size;
    // auto deserial = make_unsafe_deserializer(tx_start);
    // deserial.skip(version_size + locktime_size);
    // const auto outputs = deserial.read_size_little_endian();
    // BITCOIN_ASSERT(deserial);

    // if (index >= outputs)
    //     return{};

    // // Skip outputs until the target output.
    // for (uint32_t output = 0; output < index; ++output)
    // {
    //     deserial.skip(height_size);
    //     deserial.skip(value_size);
    //     deserial.skip(deserial.read_size_little_endian());
    //     BITCOIN_ASSERT(deserial);
    // }

    // // Read and return the target output.
    // chain::output out;
    // out.from_data(deserial, use_wire_encoding);
    // return out;

    //TODO: Fer: change this to auto& when the return type of transaction_result::transaction() be corrected!
    auto tx = transaction();

    if (index >= tx.outputs().size()) {
        return {};
    }

    return tx.outputs()[index];
}

chain::transaction transaction_result::transaction() const {
    // BITCOIN_ASSERT(slab_);
    // const auto memory = REMAP_ADDRESS(slab_);
    // const auto tx_start = memory + height_size + position_size;
    // auto deserial = make_unsafe_deserializer(tx_start);

    // // READ THE TX
    // chain::transaction tx;
    // tx.from_data(deserial, use_wire_encoding);

    // // TODO: add hash param to deserialization to eliminate this construction.
     // return chain::transaction(std::move(tx), hash_digest(hash_));

    return tx_;
}
} // namespace database
} // namespace libbitcoin
