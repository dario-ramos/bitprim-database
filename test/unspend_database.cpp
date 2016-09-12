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
#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>
#include <bitcoin/database.hpp>

using namespace boost::system;
using namespace boost::filesystem;
using namespace bc;
using namespace bc::chain;
using namespace bc::database;

#define DIRECTORY "unspend_database"

class unspend_database_directory_setup_fixture
{
public:
    unspend_database_directory_setup_fixture()
    {
        error_code ec;
        remove_all(DIRECTORY, ec);
        BOOST_REQUIRE(create_directories(DIRECTORY, ec));
    }

    ////~unspend_database_directory_setup_fixture()
    ////{
    ////    error_code ec;
    ////    remove_all(DIRECTORY, ec);
    ////}
};

BOOST_FIXTURE_TEST_SUITE(database_tests, unspend_database_directory_setup_fixture)

BOOST_AUTO_TEST_CASE(unspend_database__test)
{
    chain::output_point key1{ hash_literal("4129e76f363f9742bc98dd3d40c99c9066e4d53b8e10e5097bd6f7b5059d7c53"), 110 };
    chain::output_point key2{ hash_literal("eefa5d23968584be9d8d064bcf99c24666e4d53b8e10e5097bd6f7b5059d7c53"), 4 };
    chain::output_point key3{ hash_literal("4129e76f363f9742bc98dd3d40c99c90eefa5d23968584be9d8d064bcf99c246"), 8 };
    chain::output_point key4{ hash_literal("80d9e7012b5b171bf78e75b52d2d149580d9e7012b5b171bf78e75b52d2d1495"), 9 };

    chain::input_point value1{ hash_literal("4742b3eac32d35961f9da9d42d495ff1d90aba96944cac3e715047256f7016d1"), 1 };
    chain::input_point value2{ hash_literal("d90aba96944cac3e715047256f7016d1d90aba96944cac3e715047256f7016d1"), 2 };
    chain::input_point value3{ hash_literal("3cc768bbaef30587c72c6eba8dbf6aeec4ef24172ae6fe357f2e24c2b0fa44d5"), 3 };
    chain::input_point value4{ hash_literal("4742b3eac32d35961f9da9d42d495ff13cc768bbaef30587c72c6eba8dbf6aee"), 4 };

    data_base::touch_file(DIRECTORY "/unspend");
    unspend_database db(DIRECTORY "/unspend");
    BOOST_REQUIRE(db.create());

    db.store(key1, value1);
    db.store(key2, value2);
    db.store(key3, value3);

    // Test fetch.
    const auto unspend1 = db.get(key1);
    BOOST_REQUIRE(unspend1.valid);
    BOOST_REQUIRE(unspend1.hash == value1.hash);
    BOOST_REQUIRE_EQUAL(unspend1.index, value1.index);

    const auto unspend2 = db.get(key2);
    BOOST_REQUIRE(unspend2.valid);
    BOOST_REQUIRE(unspend2.hash == value2.hash);
    BOOST_REQUIRE_EQUAL(unspend2.index, value2.index);

    const auto unspend3 = db.get(key3);
    BOOST_REQUIRE(unspend3.valid);
    BOOST_REQUIRE(unspend3.hash == value3.hash);
    BOOST_REQUIRE_EQUAL(unspend3.index, value3.index);

    // Record shouldnt exist yet.
    BOOST_REQUIRE(!db.get(key4).valid);

    // Delete record.
    db.remove(key3);
    BOOST_REQUIRE(!db.get(key3).valid);

    // Add another record.
    db.store(key4, value4);

    // Fetch it.
    const auto unspend4 = db.get(key4);
    BOOST_REQUIRE(unspend4.valid);
    BOOST_REQUIRE(unspend4.hash == value4.hash);
    BOOST_REQUIRE_EQUAL(unspend4.index, value4.index);
    db.sync();
}

BOOST_AUTO_TEST_SUITE_END()
