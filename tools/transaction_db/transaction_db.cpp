#include <iostream>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <bitcoin/database.hpp>

using namespace boost;
using namespace bc;
using namespace bc::database;

void show_help()
{
    std::cout << "Usage: transaction_db COMMAND MAP [ARGS]" << std::endl;
    std::cout << std::endl;
    std::cout << "The most commonly used transaction_db commands are:" << std::endl;
    std::cout << "  initialize_new  " << "Create a new transaction_database" << std::endl;
    std::cout << "  get             " << "Fetch transaction by hash" << std::endl;
    std::cout << "  store           " << "Store a transaction" << std::endl;
    std::cout << "  help            " << "Show help for commands" << std::endl;
}

void show_command_help(const std::string& command)
{
    if (command == "initialize_new")
    {
        std::cout << "Usage: transaction_db " << command << " MAP "
            << "" << std::endl;
    }
    else if (command == "get")
    {
        std::cout << "Usage: transaction_db " << command << " MAP "
            << "HASH" << std::endl;
    }
    else if (command == "store")
    {
        std::cout << "Usage: transaction_db " << command << " MAP "
            << "HEIGHT INDEX TXDATA" << std::endl;
    }
    else if (command == "remove")
    {
        std::cout << "Usage: transaction_db " << command << " MAP "
            << "HASH" << std::endl;
    }
    else
    {
        std::cout << "No help available for " << command << std::endl;
    }
}

template <typename Uint>
bool parse_uint(Uint& value, const std::string& arg)
{
    try
    {
        value = lexical_cast<Uint>(arg);
    }
    catch (const bad_lexical_cast&)
    {
        std::cerr << "transaaction_db: bad value provided." << std::endl;
        return false;
    }
    return true;
}

int main(int argc, char** argv)
{
    typedef std::vector<std::string> string_list;

    if (argc < 2)
    {
        show_help();
        return -1;
    }

    const std::string command = argv[1];

    if (command == "help" || command == "-h" || command == "--help")
    {
        if (argc == 3)
        {
            show_command_help(argv[2]);
            return 0;
        }

        show_help();
        return 0;
    }

    if (argc < 3)
    {
        show_command_help(command);
        return -1;
    }

    string_list args;
    const std::string map_filename = argv[2];

    for (int i = 3; i < argc; ++i)
        args.push_back(argv[i]);

    if (command == "initialize_new")
        store::create(map_filename);

    transaction_database db(map_filename, 1000, 50);

    if (command == "initialize_new")
    {
        const auto result = db.create();
        BITCOIN_ASSERT(result);
    }
    else if (command == "get")
    {
        if (args.size() != 1)
        {
            show_command_help(command);
            return -1;
        }

        hash_digest hash;
        if (!decode_hash(hash, args[0]))
        {
            std::cerr << "Couldn't read transaction hash." << std::endl;
            return -1;
        }

        db.open();
        auto result = db.get(hash, max_size_t);
        if (!result)
        {
            std::cout << "Not found!" << std::endl;
            return -1;
        }

        const data_chunk data = result.transaction().to_data();

        std::cout << "height: " << result.height() << std::endl;
        std::cout << "position: " << result.position() << std::endl;
        std::cout << "tx: " << encode_base16(data) << std::endl;
    }
    else if (command == "store")
    {
        if (args.size() != 3)
        {
            show_command_help(command);
            return -1;
        }

        size_t height;
        if (!parse_uint(height, args[0]))
            return -1;

        size_t index;
        if (!parse_uint(index, args[1]))
            return -1;

        data_chunk data;
        if (!decode_base16(data, argv[2]))
        {
            std::cerr << "data is not valid" << std::endl;
            return -1;
        }

        chain::transaction tx;
        if (!tx.from_data(data))
            throw end_of_stream();

        const auto result = db.open();
        BITCOIN_ASSERT(result);

        db.store(height, index, tx);
        db.synchronize();
    }
    else if (command == "remove")
    {
        if (args.size() != 1)
        {
            show_command_help(command);
            return -1;
        }

        hash_digest hash;
        if (!decode_hash(hash, args[0]))
        {
            std::cerr << "Couldn't read transaction hash." << std::endl;
            return -1;
        }

        const auto result = db.open();
        BITCOIN_ASSERT(result);

        db.unlink(hash);
        db.synchronize();
    }
    else
    {
        std::cout << "transaction_db: '" << command
            << "' is not a transaction_db command. "
            << "See 'transaction_db --help'." << std::endl;
        return -1;
    }
    return 0;
}

