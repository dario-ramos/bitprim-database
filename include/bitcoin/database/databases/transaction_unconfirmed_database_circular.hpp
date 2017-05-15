/**
 * Copyright (c) 2011-2017 Bitprim developers (see AUTHORS)
 *
 * This file is part of Bitprim.
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
#ifndef BITPRIM_DATABASE_TRANSACTION_UNCONFIRMED_DATABASE_CIRCULAR_HPP
#define BITPRIM_DATABASE_TRANSACTION_UNCONFIRMED_DATABASE_CIRCULAR_HPP

#include <cstddef>
#include <memory>
#include <boost/filesystem.hpp>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/define.hpp>
//#include <bitcoin/database/memory/memory_map.hpp>
//#include <bitcoin/database/result/transaction_result.hpp>
//#include <bitcoin/database/primitives/slab_hash_table.hpp>
//#include <bitcoin/database/primitives/slab_manager.hpp>
//#include <bitcoin/database/unspent_outputs.hpp>

namespace libbitcoin {
namespace database {


class BCD_API transaction_unconfirmed_database_circular
{
public:
//    typedef boost::filesystem::path path;
//    typedef std::shared_ptr<shared_mutex> mutex_ptr;

    /// Sentinel for use in tx position to indicate unconfirmed.
    static const size_t unconfirmed;

    /// Construct the database.
    transaction_unconfirmed_database_circular(size_t tx_limit);

    /// Close the database (all threads must first be stopped).
    ~transaction_unconfirmed_database_circular();

    /// Initialize a new transaction database.
    bool create();

    /// Call before using the database.
    bool open();

    /// Call to unload the memory map.
    bool close();

    /// Fetch transaction by its hash, at or below the specified block height.
//    transaction_result get(hash_digest const& hash) const;
    boost::optional<chain::transaction> get(hash_digest const& hash) const;


    /// Get the output at the specified index within the transaction.
    // bool get_output(chain::output& out_output, size_t& out_height,
    //     bool& out_coinbase, const chain::output_point& point,
    //     size_t fork_height, bool require_confirmed) const;

    /// Store a transaction in the database.
    void store(chain::transaction const& tx);

    // /// Update the spender height of the output in the tx store.
    // bool spend(const chain::output_point& point, size_t spender_height);

    // /// Update the spender height of the output in the tx store.
    // bool unspend(const chain::output_point& point);

    // /// Promote an unconfirmed tx (not including its indexes).
    // bool confirm(const hash_digest& hash, size_t height, size_t position);

    // /// Demote the transaction (not including its indexes).
    // bool unconfirm(const hash_digest& hash);

    /// Commit latest inserts.
    void synchronize();

    /// Flush the memory map to disk.
    bool flush() const;

    bool unlink(hash_digest const& hash);
    bool unlink_if_exists(hash_digest const& hash);

    template <typename UnaryFunction>
    void for_each(UnaryFunction f) const {
        for (auto const & x : data_) {
            //std::cout << "Key:[" << n.first << "] Value:[" << n.second << "]\n";
            if (!f(n.second)) {
                return;
            }
        }
    }

private:
    std::pair<bool, uint64_t> total_input_value(chain::transaction const& tx) const;
    std::pair<bool, uint64_t> fees(chain::transaction const& tx) const;

    using value_t = std::pair<chain::transaction, double>;
    std::vector<value_t> data_;
};

} // namespace database
} // namespace libbitcoin

#endif //BITPRIM_DATABASE_TRANSACTION_UNCONFIRMED_DATABASE_CIRCULAR_HPP
