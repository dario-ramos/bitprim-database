#include <iostream>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <bitcoin/database.hpp>

using namespace boost;
using namespace bc;
using namespace bc::database;

void show_help() {
    std::cout << "Usage: build_utxo COMMAND MAP [ARGS]" << std::endl;
    std::cout << std::endl;
    std::cout << "The most commonly used build_utxo commands are:" << std::endl;
    std::cout << "  initialize_new  " << "Create a new transaction_database" << std::endl;
    std::cout << "  get             " << "Fetch transaction by hash" << std::endl;
    std::cout << "  store           " << "Store a transaction" << std::endl;
    std::cout << "  help            " << "Show help for commands" << std::endl;
}

void show_command_help(const std::string& command) {
    if (command == "initialize_new")
    {
        std::cout << "Usage: build_utxo " << command << " MAP "
            << "" << std::endl;
    }
    else if (command == "get")
    {
        std::cout << "Usage: build_utxo " << command << " MAP "
            << "HASH" << std::endl;
    }
    else if (command == "store")
    {
        std::cout << "Usage: build_utxo " << command << " MAP "
            << "HEIGHT INDEX TXDATA" << std::endl;
    }
    else if (command == "remove")
    {
        std::cout << "Usage: build_utxo " << command << " MAP "
            << "HASH" << std::endl;
    }
    else
    {
        std::cout << "No help available for " << command << std::endl;
    }
}

template <typename Uint>
bool parse_uint(Uint& value, const std::string& arg) {
    try {
        value = lexical_cast<Uint>(arg);
    } catch (const bad_lexical_cast&) {
        std::cerr << "transaaction_db: bad value provided." << std::endl;
        return false;
    }
    return true;
}

int main(int argc, char** argv) {

    typedef std::vector<std::string> string_list;

    // if (argc < 2) {
    //     show_help();
    //     return -1;
    // }

    // const std::string command = argv[1];

    // if (command == "help" || command == "-h" || command == "--help") {
    //     if (argc == 3) {
    //         show_command_help(argv[2]);
    //         return 0;
    //     }

    //     show_help();
    //     return 0;
    // }

    // if (argc < 3) {
    //     show_command_help(command);
    //     return -1;
    // }

    string_list args;
    const std::string utxo_filename = argv[1];
    const std::string tx_filename = argv[2];
    const std::string spend_filename = argv[3];

    for (int i = 3; i < argc; ++i)
        args.push_back(argv[i]);

    // if (command == "initialize_new")
    //     data_base::touch_file(tx_filename);

    data_base::touch_file(utxo_filename);

    transaction_database tx_db(tx_filename);
    unspent_database utxo_db(utxo_filename);
    spend_database spend_db(spend_filename);


    const auto utxo_db_create_result = utxo_db.create();
    BITCOIN_ASSERT(utxo_db_create_result);
    const auto tx_db_start_result   = tx_db.start();
    BITCOIN_ASSERT(tx_db_start_result);
    const auto utxo_db_start_result = utxo_db.start();
    BITCOIN_ASSERT(utxo_db_start_result);
    const auto spend_db_start_result = spend_db.start();
    BITCOIN_ASSERT(spend_db_start_result);



    auto item_data = tx_db.get_first_item();

    while (std::get<2>(item_data)) {
        auto result = std::get<2>(item_data);

        const auto tx = result.transaction();
        const data_chunk data = tx.to_data();

        std::cout << "height: " << result.height() << std::endl;
        std::cout << "index: " << result.index() << std::endl;
        std::cout << "tx: " << encode_base16(data) << std::endl;

        for (const auto& input: tx.inputs) {

            const auto spend = spend_db.get(input.previous_output);

            if (!spend.valid) {
                std::cout << "Not found in Spend_DB, storing un UTXO DB: "
                          << encode_hash(spend.hash) << ":" << spend.index << std::endl;

                // void store(chain::output_point const& outpoint);
                utxo_db.store(input.previous_output);
                utxo_db.sync();

                // input.to_data(sink);
                // hash_digest hash;
                // uint32_t index;

                // input.previous_output.hash
                // input.previous_output.index
            }
        }


        item_data = tx_db.get_next_item(std::get<0>(item_data), std::get<1>(item_data));        
    }





    // if (command == "initialize_new")
    // {
    //     const auto result = db.create();
    //     BITCOIN_ASSERT(result);
    // }
    // else if (command == "get")
    // {
    //     if (args.size() != 1)
    //     {
    //         show_command_help(command);
    //         return -1;
    //     }

    //     hash_digest hash;
    //     if (!decode_hash(hash, args[0]))
    //     {
    //         std::cerr << "Couldn't read transaction hash." << std::endl;
    //         return -1;
    //     }

    //     db.start();
    //     auto result = db.get(hash);
    //     if (!result)
    //     {
    //         std::cout << "Not found!" << std::endl;
    //         return -1;
    //     }

    //     const data_chunk data = result.transaction().to_data();

    //     std::cout << "height: " << result.height() << std::endl;
    //     std::cout << "index: " << result.index() << std::endl;
    //     std::cout << "tx: " << encode_base16(data) << std::endl;
    // }
    // else if (command == "store")
    // {
    //     if (args.size() != 3)
    //     {
    //         show_command_help(command);
    //         return -1;
    //     }

    //     size_t height;
    //     if (!parse_uint(height, args[0]))
    //         return -1;

    //     size_t index;
    //     if (!parse_uint(index, args[1]))
    //         return -1;

    //     data_chunk data;
    //     if (!decode_base16(data, argv[2]))
    //     {
    //         std::cerr << "data is not valid" << std::endl;
    //         return -1;
    //     }

    //     chain::transaction tx;
    //     if (!tx.from_data(data))
    //         throw end_of_stream();

    //     const auto result = db.start();
    //     BITCOIN_ASSERT(result);

    //     db.store(height, index, tx);
    //     db.sync();
    // }
    // else if (command == "remove")
    // {
    //     if (args.size() != 1)
    //     {
    //         show_command_help(command);
    //         return -1;
    //     }

    //     hash_digest hash;
    //     if (!decode_hash(hash, args[0]))
    //     {
    //         std::cerr << "Couldn't read transaction hash." << std::endl;
    //         return -1;
    //     }

    //     const auto result = db.start();
    //     BITCOIN_ASSERT(result);

    //     db.remove(hash);
    //     db.sync();
    // }
    // else
    // {
    //     std::cout << "build_utxo: '" << command
    //         << "' is not a build_utxo command. "
    //         << "See 'build_utxo --help'." << std::endl;
    //     return -1;
    // }


    return 0;
}

