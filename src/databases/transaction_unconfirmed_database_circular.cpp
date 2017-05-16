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
#include <bitcoin/database/databases/transaction_unconfirmed_database_circular.hpp>

#include <cstddef>
#include <cstdint>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/result/transaction_result.hpp>

namespace libbitcoin {
namespace database {

using namespace bc::chain;
using namespace bc::machine;

//static constexpr auto value_size = sizeof(uint64_t);
//static constexpr auto version_size = sizeof(uint32_t);
//static constexpr auto locktime_size = sizeof(uint32_t);
//static constexpr auto version_lock_size = version_size + locktime_size;
//const size_t transaction_unconfirmed_database_circular::unconfirmed = max_uint32;

// Transactions uses a hash table index, O(1).
transaction_unconfirmed_database_circular::transaction_unconfirmed_database_circular(size_t max_capacity) {
    data_.reserve(max_capacity);
}

transaction_unconfirmed_database_circular::~transaction_unconfirmed_database_circular() {
    close();
}

// Create.
// ----------------------------------------------------------------------------

// Initialize files and start.
bool transaction_unconfirmed_database_circular::create() {
    return true;
}

// Startup and shutdown.
// ----------------------------------------------------------------------------

bool transaction_unconfirmed_database_circular::open() {
    return true;
}

bool transaction_unconfirmed_database_circular::close() {
    return true;
}

// Commit latest inserts.
void transaction_unconfirmed_database_circular::synchronize() {}

// Flush the memory map to disk.
bool transaction_unconfirmed_database_circular::flush() const {
    return true;
}

// Queries.
// ----------------------------------------------------------------------------
//transaction_result transaction_unconfirmed_database_circular::get(hash_digest const& hash) const
boost::optional<chain::transaction> transaction_unconfirmed_database_circular::get(hash_digest const& hash) const  {
    boost::lock_guard<boost::mutex> guard(mutex_);

    auto it = std::find_if(data_.begin(), data_.end(), [&hash](value_t const& x) {
        return x.first.hash() == hash;
    });

    if (it == data_.end()) return boost::optional<chain::transaction>();

    return boost::optional<chain::transaction>(it->first);
}

void transaction_unconfirmed_database_circular::store(chain::transaction const& tx) {


    boost::lock_guard<boost::mutex> guard(mutex_);

    std::cout << "TX to store hash: " << libbitcoin::encode_hash(tx.hash()) << "\n";
//    auto const fee = fees(tx).second;
    auto const fee = tx.fees(); //TODO: see how to get this value, correct!
    size_t const tx_weight = tx.to_data(true).size();
    auto const tx_fee_per_weight = double(fee) / tx_weight;

    auto const cmp = [](value_t const& x, double fpw) {
        return x.second > fpw;
    };

    auto const f = std::lower_bound(data_.begin(), data_.end(), tx_fee_per_weight, cmp);

    if (data_.size() == data_.capacity()) {
        if (f != data_.end()) {
            if (f == data_.end() - 1) { //is the last element
                *f = std::make_pair(tx, tx_fee_per_weight);
            } else {
                data_.pop_back();
                data_.insert(f, std::make_pair(tx, tx_fee_per_weight));
            }
        }
    } else {
        data_.insert(f, std::make_pair(tx, tx_fee_per_weight));
    }
}

// bool transaction_unconfirmed_database_circular::spend(const output_point& point, size_t spender_height)
// bool transaction_unconfirmed_database_circular::unspend(const output_point& point)
// bool transaction_unconfirmed_database_circular::confirm(const hash_digest& hash, size_t height, size_t position)
// bool transaction_unconfirmed_database_circular::unconfirm(const hash_digest& hash)

bool transaction_unconfirmed_database_circular::unlink(hash_digest const& hash) {
    //return data_.erase(hash) == 1;

    boost::lock_guard<boost::mutex> guard(mutex_);

    auto const it = std::remove_if(data_.begin(), data_.end(), [&hash](value_t const& x){
        return x.first.hash() == hash;
    });
    bool res = it != data_.end();
    data_.erase(it, data_.end());
    return res;
}

//bool transaction_unconfirmed_database_circular::unlink_if_exists(hash_digest const& hash) {
//    return data_.erase(hash) == 1;
//}


//std::pair<bool, uint64_t> transaction_unconfirmed_database_circular::total_input_value(chain::transaction const& tx) const{
//
//    uint64_t total = 0;
//
//    for (auto const& input : tx.inputs()) {
//        libbitcoin::chain::output out_output;
//        size_t out_height;
//        bool out_coinbase;
//        if (!get_output(out_output, out_height, out_coinbase, input.previous_output(), libbitcoin::max_size_t, false)){
////            std::cout << "Output not found. Hash = " << libbitcoin::encode_hash(tx.hash())
////                      << ".\nOutput hash = " << encode_hash(input.previous_output().hash())
////                      << ".\nOutput index = " << input.previous_output().index() << "\n";
//            return std::make_pair(false, 0);
//        }
//        const bool missing = !out_output.is_valid();
//        total = ceiling_add(total, missing ? 0 : out_output.value());
//    }
//
//    return std::make_pair(true, total);
//}
//
//std::pair<bool, uint64_t> transaction_unconfirmed_database_circular::fees(chain::transaction const& tx) const {
//    auto input_value = total_input_value(tx);
//    if (input_value.first){
//        return std::make_pair(true, floor_subtract(input_value.second, tx.total_output_value()));
//    }
//    return std::make_pair(false, 0);
//}

} // namespace database
} // namespace libbitcoin
