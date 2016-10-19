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
            } else {
                std::cout << "Found in Spend_DB, ignoring: "
                          << encode_hash(spend.hash) << ":" << spend.index << std::endl;
            }
        }


        item_data = tx_db.get_next_item(std::get<0>(item_data), std::get<1>(item_data));        
    }

    auto bucket = std::get<0>(item_data);
    auto file_offset = std::get<1>(item_data);
    auto tx_result = std::get<2>(item_data);
    auto tx_result_valid = bool(tx_result);


    std::cout << "tx_result_valid: " << tx_result_valid << std::endl;
    std::cout << "bucket: " << bucket << std::endl;
    std::cout << "bucket: " << bucket << std::endl;
    std::cout << "bucket: " << bucket << std::endl;

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




// height: 430382
// index: 2291
// tx: 01000000000000000294

// height: 412630
// index: 13
// tx: 01000000014068e46f2953fdb3149199629db7c74df940751481da91ff0c2dc08a45ad82b701000000db00483045022100dc3eff46dbc933f809dc8b6212208b0dac2ffcd0bd7e348a7faf5a0351d462e502203a13a12daa998437dc6228e33c5ff0152c96905ef11c81b1e919d5eed464d5dd014830450221008625948cf7e5a0c1c96a9e8171b97e6810d60bb791ad3b3433ed9177929c07c102206b308c5090600f89d81ec115520ebdd58e0f088323bdba38c8f7f8478ee81b0e0147522102149363fc2dff6e6d47d81984844c8eb98cc8b78fbd191b04981d82bcb75979c02102ef1287b9054dea32f99420c5916268739ef5c29abe7d691f720795f1bb10c33152aeffffffff0238c10100000000001976a914c1b0e67ae1c558fb1ad919b630723b4e6522ead088acbad416000000000017a914c0b440ac1190a3a15e84309da36cc2705087b7428700000000

// height: 401818
// index: 86
// tx: 010000000144cc6a91c7fc9e17653e4a5eccb2312c8cf27671d7e21b5282352ba6bf047e37000000006b48304502210095142ae9a09bef8fbfdce0e529ff9fda2201ecae380c66fd63bda239cd8cf8b902206c148ee6e5d74823729652d993f8e76972125171ceb5d92ce633deb47e79a7c60121032261b6230ae91206e30180fb9e3ce99c1bb37c64275fdb7eb6e1fa4ff2cb6449feffffff02026a2e34000000001976a91497f58195fcb710c3e3a04ffca72e8a212483787c88ac00ca9a3b0000000017a914a08203781adc643af60bea85e10150638544b84d878f210600

// height: 400616
// index: 355
// tx: 01000000026a211f0d69a6563087eaf1dfe52098844ff008c8270a1e5bc12bfc255ad1f3ec010000006a47304402202c28bc270033e8d5171a62ed2ce6d39d4e644ac8f7dd42771e1c0772e4890bf602205be270e6f0f388eaec14cb217a2304bc68e231cc4450235a751b2929125e20640121035787390183eb87a9954fad74ddf47795799ee7faea2ebcdb0e831530aec852d4ffffffff5b51830bfdda07281c694d94c962b9b7a010ad4430271fe9142cb9b6ddc696ce010000006b483045022100b66f52ae3cb7f3b43cc8cbb38fa9213007158cb57fcf8595dcafbc32f817789a02204d5fcec4c2fe430fe0baf4f2299208a05d2f1f47817b44de9924ff3992c6e504012103ffc9559fc2fc305f70da6c0e975ebb2e039a6a876bf48d7432a094f5a5eefea3ffffffff03ced33300000000001976a914025a52b8209dddfb6afd76e16380da9c020ff4b388acb9010200000000001976a9145766d478e04fca7d83e70f3c70525bebc841cfc788acbc180200000000001976a914e2338345c82654cb7b194f37edcbe3884ae1400188ac00000000

// height: 383006
// index: 320
// tx: 0100000002e4bb40045df4ed4dbb779b0ff2f54801815abf4f076fc29e40972865ccdeb92d000000006a47304402206e4f6853c40dd8343b84af53bc970e100e24d4009a3f161b9caaf4670339a83802207da0e1a08ae0bf69b9b8f3c9493e758ad4ac65f46ac5b1fe4c4535225623c320012102cd0951c63abff757c7c75fb6c925d8c3074ba815234d3361501778c6b5069b43ffffffff4c43c744f95e2c2aab125ba4946a4fedc3502ed2935d4b4462372edde162eb10010000006a473044022064e645c149a5ad3c5bfabc8d47b711fce2a461f6190c5b45aee6ae203a0b9b1502203801210f3e162250f77cb7f9e6387cd3c490b4d6875616ed4ea050b45f3b5c65012103ef966eb19a9181a0534c561a464259a8aed7034470d79ad4eb5e9152bd5a7d1cffffffff028007d00c000000001976a914899054003423b725a495b6fe58721ab21647691d88acf7195c01000000001976a914cf6d1e75b547ec92277590ebf4ed04b9e976b58e88ac00000000

// height: 283629
// index: 477
// tx: 01000000020262eb697a3819b7ddcee4763d1a7197098ecef8817e58016947b2dcbaf92a79010000008b48304502205c46ad0024aaf3155a7904cf3ab4050158d542a6b5470c6c9d30e139d7c4ac4e022100bcb69dd780429c16a81bdf59cb4194c3354d6f5fcf21490538cb367b872094800141043f4f4eaa9721949c622c2bab0b0f2a00ce3faa9299d1c21453c75de73bc7b250b545f25707b56bcf140b24b636fc96a1f0524effe0a7606402597bdd5ed53e2affffffffc6c2efca4a49434f64e9299205c563f93e9f6a4137ba3f4d52801342bee010a2010000008b4830450220075d9fc8d07cc749a4d9d7fc51c81df14cfee04cfbbe5d72faa56fbe3ec6a011022100edfbc9b458af260167286a48eb91038a5cbf7449838fc3ff6724fddb30d0c3b90141043f4f4eaa9721949c622c2bab0b0f2a00ce3faa9299d1c21453c75de73bc7b250b545f25707b56bcf140b24b636fc96a1f0524effe0a7606402597bdd5ed53e2affffffff02a6a2af00000000001976a914a2956ca21a3a8c602f51621682df43d0e59911bc88ac327f1600000000001976a914657e49d2cb20e68f2f6c9bf5234f941674eeeda488ac00000000

// height: 244593
// index: 55
// tx: 0100000001d2c5fd6c1840f3031e871906bb2e25ae88524664f61259fef6cb83969a2a88ae010000008a473044022064d32cb3b2f56d130199f3ff77b76be1c6f97ec9d3bdfffbe2a5e41a41e9e3da02207f88e4bad329e474101e4227f23ddae557f83303ebd4473fd9f40f314bccf2e9014104b36d633ee9aed34d2cbc6079504e18a9f0fe207fed6e13506ca7f2aa32b401545147b3f5649a84c0759b160c3f62c08d46ecca4764931248bd8ad4e5e898d242ffffffff0220521144010000001976a91409bf770cc15e796acda7d01d3009ef2a5718055588ac6060cc04000000001976a914b81d0953edf64e83127e247f06477143b9a9b54788ac00000000

// height: 222066
// index: 81
// tx: 010000000127523dbc7ec7bad5a1c9a60b8502fa48d2bec2271609e594e150c5a974841df3010000008b483045022100cc5e72e5aa61454d8c792f1556dd57af83be095b986cfacd86caeb46e2b54886022005ef572e90aa24b50fcc2375a6b52afbcea5c42294cb5c4c3b29c4dcfd917efe01410470fbf3684f1b3dc3142052102cde89e2e289a3877383feb8f01437b7522b87ca6684969b175d1f41de1791bce81c410db8a0bf32839de072a84933eae15d9957ffffffff020024f400000000001976a91406f1b6703d3f56427bfcfd372f952d50d04b64bd88acf0111102000000001976a91465f6125582f96793b4426a2baea4737be2fed35e88ac00000000

// height: 413566
// index: 6
// tx: 0100000001b582609cd20761e8e011217b1e2541cbe3113901e093fd03650d13151201c3da000000006b48304502210092501cf80b54ad50f9c0ac3203af20b8a35c69a47577f4abf1f77e4a6a68f8b4022053d5fc6e9557c5fb6845e42c2707300b0bd9b35e4ee7a001989166a0cfeea7be012102163e6df00c8507259760b2f2dff7328c0962767ca163c1619195d3918d63e9d8feffffff02b0501201000000001976a91413aa49e6154506d035437e974ca88b866ae607c788aca85c7008000000001976a914c04726a47addfbee92daefc2e1d8ed52958f74ae88ac734f0600

// height: 323696
// index: 258
// tx: 010000000249f606559fe84cdc77689b0ddf9c72cceefe7d7bbcc649575a2d8557097d1169010000008a47304402201f264656f0742c47cd70d90656861a1d760862c7bfaa52912a78add246512793022053340964b7228675b3b29046edf24cab7f8c537479370efb302e151c4a44fdef01410491e679f4336a317e60f7cfa9a6c15f5dd45a12e7e2e676f4b5b44ead62dabb868f08ac3629ce15085427fc93d6c1028d611293eadf536954b7eced9b5c147e6bfffffffff4eda0beb091be4acd771aaf8a49707bad437facd29fd5f2397d6685817331ca020000008a47304402205d93c3edac28691dddc19ac47dae3babe37866dad08d6e6c91894872d0358af502203359b15d9372656e1426c729f28de8411fe9818019403cf905cde7e708d9a8870141040c2d87745fb4e640f068adb021df859117e9f3652a7ac47b014461c779aad757e9e483c6a56bc4e72a6275cd3e73337c05b667981ef84c7d9ccfdfa7e1733985ffffffff031b21f300000000001976a914519a719c684cf092627fcb5cf0cd76ff010963f988aca5660000000000001976a9146a165666c26ca2f1db1262895e7c5b33b8c7617d88ac22630200000000001976a914eaa1146f5d87688556593521542e0065a2619a9388ac00000000

// height: 305189
// index: 295
// tx: 0100000001ec460e77bc7e5792ff0b72e9e5440fb721de5839eb4ecf9309d646fe0d77cf74010000006b483045022100b8d2442dde11f2c8e6310654aa342afc23cb5914f7f269f732514102e3c1d13f022014b25cf9aa4e5ff439f44afc92732b68f8968fb4794b7cb8366d18a459d56b9a0121038b625fe515cb16e95a8511ab26fe8579eb21fa56dc71576d776bfbb834ea4e15ffffffff02c9646f22000000001976a91438494275da2a87b440e415066b90894c6c2c3dd288ac20b38100000000001976a9143ea0fabc53548f6985941fa3122e7b0d48cdb99d88ac00000000

// height: 382327
// index: 371
// tx: 01000000017a95f6928ce5fea9daebd957540776386c35dd29b363112a9f3adc58ceedb0b4010000006b483045022100c435eecf4bbc2f4ac8d05972c019f03f43bb4341cb47f4a2c4afb667fbe6948902200c1226a4ad6a47513d09c97cb1a0633be6a62f290fe6b4c283088ad9fc33e65701210226b0e2cfcc85cfad42eaa6c1e98f1ff71863ae4d42f326c792e04f8e5ded60f2feffffff02b40c513e000000001976a914441689dcd384f19cb0f345bdbebef93bc0b0a06c88ac3c116302110000001976a914d28086cb74777d36add3c0dc67c9e001983ea5d188ac6cd50500

// height: 284830
// index: 1153
// tx: 01000000021e04797c72586497ccd6b8ac6e70b3643debb82f205c47e9a5d8d173cb4cfc33000000006c493046022100bd761c293d0e12d2bf0046c6300c6aa2a3a4eb9c4e3855727744647185475fc6022100c2f53328f938b4c1824a1d187d1411512412c4553440194fe8bc03d05352080f0121024f9c3dbc5d9da464ce20ae6a33bf5d9c8a11d784d44794914fd1718aae07bbcaffffffff29bfeda4bfd2f8af2f60d8a85fc813929c54787a80ec79923d2cfb8a55d1f486000000006c493046022100b66d18c40dca395664af4c4808374b3b07e433141fa80363b3c3410b86dc5652022100db76d4f1cb4ddb9d725d6299a7f52f97f7a9657828c37cdb0ba9dec9087e3e0b0121024f9c3dbc5d9da464ce20ae6a33bf5d9c8a11d784d44794914fd1718aae07bbcaffffffff02a0252600000000001976a914090a9c6db64f8d11e4cd36accadb5024c3ad293588ac20871c00000000001976a914401aa09371cc3ff38d132de6616cd43a2d7c7f2d88ac00000000

// height: 317529
// index: 14
// tx: 0100000001ed6e00d6e9f171309646bb2a6badb6c1147b5ddfdfe81f2260bca5bb44ec25c6010000006a473044022064d1662d672f3227698e60f7676e7f8c77107f315010a9f441faf39453ca26d7022044fa93a783df438a029e90610286633525b068583399f235881d046169d6a020012103acbeaf5e0adc049523d1016a2ef868ef3464d8d120c6d3b3ecaf6de953fa76e5ffffffff02c00e1602000000001976a9145549b992f4173ba346b9e552250128acf6decc3988ac20fd4b00000000001976a91423f75aa88d891cd9aefcddc4672f2f0679d2569588ac00000000

// height: 237561
// index: 489
// tx: 0100000002b350ae1b4669580827f535b60b7ee0f0ad142fbb7af02c5fa8cb8218bc4b452d000000008c493046022100a4933a50fd5ddfacce3616251d31f2f2da3b392e810b6ad5fb53c5fef6d71c3b022100a08d373f4175ae6c2f1f20a6b13bbd1e82e065741afc35a386a9eb83f753ac4b0141043141a0553feb4f114c668de675af69156f9ed333eb79a1f47100137ae3c76f0a1ba8f8f873bd1d08126ebacfe440b8e1397f71e7c957e9a0aa074753dcfe6161ffffffffc8b43f2ecf8b655b0fecf9eff83e8a9b6d19ee1255ccc6aae47198efb4950b75010000008b483045022100ba96382627d2a89e5ab7c2bb49160f0626351b23322bbc5d841f1f218c78d855022020a78b72bc7b1d95b932154ac9bf9871d47eaa7fc3d17017c46c3e3f8b88507e0141047070110e6c58e6d86dadd0c49c114e17e6f58087a572d94b3b1b1db9d18c781518b18a9266c5d5a504f57f7710e7d5feb213626e0cbd13cfaf44f30163794600ffffffff0200ca9a3b000000001976a914db2abe3e0a7698de2a4ee6115d1311df71f9e35588acea801600000000001976a9143c7bc24d522cf6bba64c967a030f61720449d43888ac00000000

// height: 230471
// index: 326
// tx: 0100000001578d9f9d83c07339ee2486ee662111d8951a6c3060b04bdde9f880a7aba3862e000000008b483045022100ea2c277059e583e263b562fe1b15495205aee862ebf440be957a8ba307b3f97602201753293d51dcae23647c247c41e00f74e382cbf6fc911f47f11168effc11cbeb014104475e8d8d40c7ccefd99afe6c887eb7dfc68c65656d95c6d03972d8ad834cbd9782795c3e0411d90028d0bcb8e620f94090d79fd3d6f45c650d71632197023ba2ffffffff0200093d00000000001976a914cef10139f53a5da22069630d3c72e9064319855188ac52ab1605000000001976a914b8da216e9c95245f35a16b4342031f95c6e12c8888ac00000000

// height: 378750
// index: 755
// tx: 01000000017c96cee72a062d56818891becc781dba5e6d60191235e9200b1caaa69cbeb2e8010000006b483045022100d8f9b48c373388b0c856d0bc12f0a25337d277c9dc2a9aaa9c1aa89b5445de5c02207c8527438e869a0c2adeb1360247eab0d26adce39157278bbece1198f9ee0b38012103bc264b4fd09bbe028b33292fd08dc0e361233b3d0fa0eb24869d7ca1f392e145ffffffff0270430500000000001976a914cd6340771745779ce836a8f0ed21f73549434e8788ac5e6a0c88000000001976a914a850318fd2a2f64aaec69462ac23f8fe3e04ef8988ac00000000

// height: 364899
// index: 886
// tx: 0100000006845486edc9a7fcfb42e0afa13dab9be47aa7b6a2b182145b3359f6d72caba0140b0000006a4730440220498da1e634f90fbf050c39cee69e320f7bd0bfd7b4b56a96f8cf085975cdaa7502204fa314c981ff4f9633380216e4173c93fc9e2f9157eeba93f15b9b5800055d41012102a194c156f148588c3099efa12d743d8dfb654aa73a4d0a2f421005125d8ca34cffffffff92a7896ab3fc4222fee67166e298f31a529d4fc2507165681f040af16d4dd2a2010000006a473044022030f29b5d196296d25229e846f57bb846796fbf1ab95e9aed82a79b29cc13494002202d778ccee328e539c086737cf81a115dabf096dff24ec0c8bc703c270bef2df9012103c40629eb4e3e88c4780bb7249e9d69f644326550480e89586bcf0929f0b70217ffffffff5229446166e2f35041e80b89f0efffdbf2ed8730b3b230a29421e57ed42c5308060000006a47304402205ebc03de8a39582f085e6bd2937b9ffb790a6c69a05c42716bcebda716b302c30220023de4c5f7cb2895372ce57e0eb3cb9d62742a4d08f118407b30f61f1be10058012103c40629eb4e3e88c4780bb7249e9d69f644326550480e89586bcf0929f0b70217ffffffffd783ec45eb15abd0a2831977244538de7cdf1cfd8d91147bc5740a81d962cc9d120000006b483045022100b7caa608c5d41b3b470c29b28c377a09ce2f534541187c862b5c85b78840ee4f022021d3fad05d49aa294109d79396170f7e45f65cdae774d6891d14ca752ea1b31a012103fd69ee0d74f23f68eebac78984bcc8a3f5a128a1e4cd03cb5c1c82d5a95875efffffffff898349ae0a757f578c4ead7210095de12e0e02cfabf92e1b7ac6218377ad7539130000006a47304402207d8be582702b39d5e9deb2f0b6785eda4f265c3c39f9a8f1f1bee5e895bfefaa02206c77891c01dc63f782935680d526f3f838d6fe17c427b69e17c17c4e85b54b59012102de408d7b20bfa401bf2d095fb1757ab732a340e4df5f19d1ba1a27131690376affffffff1a0552540a7b00d461f11ce4493be4152b90e9bcf0017deb910f084bc5d6b536060000006a47304402206a551010e56210d4faedc34c458074c54f3ff1bb878f0e8947e2ef4e68c2e25f022030ec5c20a107369b4cf7a8c4386133a4135e77b2027ae8d6d3c0f36130ba84a8012102e1262b45ad6dd982363b25918b1abc59e89ef6cf0b4e0221100d5f20418bc421ffffffff022034fe07000000001976a914fc0344e1cfd1e5e1edb9cd9645cf2ba81b6144d288ac6b5a2c0a000000001976a9148700fe9787c9539102a18b9a888a274bb66fc85988ac00000000
