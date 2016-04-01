#include "src/MicroCore.h"
#include "src/CmdLineOptions.h"
#include "src/tools.h"

#include "ext/minicsv.h"
#include "ext/format.h"

#include <iostream>
#include <string>
#include <vector>
#include <chrono>

using namespace std;



using boost::filesystem::path;
using boost::filesystem::is_directory;

// without this it wont work. I'm not sure what it does.
// it has something to do with locking the blockchain and tx pool
// during certain operations to avoid deadlocks.
namespace epee
{
    unsigned int g_test_dbg_lock_sleep = 0;
}



int main(int ac, const char* av[]) {

    // get command line options
    xmreg::CmdLineOptions opts {ac, av};

    auto help_opt = opts.get_option<bool>("help");

    // if help was chosen, display help text and finish
    if (*help_opt)
    {
        return 0;
    }

    // get other options
    auto start_height_opt = opts.get_option<size_t>("start-height");
    auto out_csv_file_opt = opts.get_option<string>("out-csv-file");
    auto bc_path_opt      = opts.get_option<string>("bc-path");


    // default path to monero folder
    // on linux this is /home/<username>/.bitmonero
    string default_monero_dir = tools::get_default_data_dir();

    // the default folder of the lmdb blockchain database
    // is therefore as follows
    string default_lmdb_dir   = default_monero_dir + "/lmdb";

    // get the program command line options, or
    // some default values for quick check
    size_t start_height  = start_height_opt ? *start_height_opt : 0;
    string out_csv_file  = out_csv_file_opt ? *out_csv_file_opt : "/tmp/block_access_time.csv";
    path blockchain_path = bc_path_opt ? path(*bc_path_opt) : path(default_lmdb_dir);


    if (!is_directory(blockchain_path))
    {
        cerr << "Given path \"" << blockchain_path   << "\" "
             << "is not a folder or does not exist" << " "
             << endl;
        return 1;
    }

    blockchain_path = xmreg::remove_trailing_path_separator(blockchain_path);

    cout << "Blockchain path: " << blockchain_path << endl;

    // enable basic monero log output
    uint32_t log_level = 0;
    epee::log_space::get_set_log_detalisation_level(true, log_level);
    epee::log_space::log_singletone::add_logger(LOGGER_CONSOLE, NULL, NULL);


    // create instance of our MicroCore
    xmreg::MicroCore mcore;

    // initialize the core using the blockchain path
    if (!mcore.init(blockchain_path.string()))
    {
        cerr << "Error accessing blockchain." << endl;
        return 1;
    }

    // get the highlevel cryptonote::Blockchain object to interact
    // with the blockchain lmdb database
    cryptonote::Blockchain& core_storage = mcore.get_core();

    // get the current blockchain height. Just to check
    // if it reads ok.
    uint64_t height = core_storage.get_current_blockchain_height();

    if (start_height > height)
    {
        cerr << "Given height is greater than blockchain height" << endl;
        return 1;
    }


    cout << "Current blockchain height: " << height << endl;


    // output csv file
    csv::ofstream csv_os {out_csv_file.c_str()};

    if (!csv_os.is_open())
    {
        cerr << "Cant open file: " << out_csv_file << endl;
        return 1;
    }

    cout << "Csv file: " <<  out_csv_file << " opened for wrting results." << endl;

    // write csv header
    csv_os << "Height" << "Timestamp" << "Access_time" << "Size"
           << "Hash" << "No_tx" << "Reward" << "Dificulty" << NEWLINE;

    // show command line output for everth i-th block
    const uint64_t EVERY_ith_BLOCK {2000};

    for (uint64_t i = start_height; i < height; ++i) {

        // show every nth output, just to give
        // a console some break
        if (i % EVERY_ith_BLOCK == 0)
            cout << "Analysing block " << i <<  "/" << height << endl;



        crypto::hash block_id;

        try
        {
            // get block hash to be used to for the search
            // in the next step
            block_id = core_storage.get_block_id_by_height(i);
        }
        catch (const exception& e)
        {
            cerr << e.what() << endl;
            continue;
        }


        cryptonote::block blk;

        try
        {
            // measure time of accessing ith block from the blockchain
            auto start = chrono::system_clock::now();
            //blk = core_storage.get_db().get_block_from_height(i); // <-- alternative, faster
            core_storage.get_block_by_hash(block_id, blk);
            auto duration = chrono::duration_cast<chrono::nanoseconds>(chrono::system_clock::now() - start);

            // get block size
            size_t blk_size = core_storage.get_db().get_block_size(i);

            if (i % EVERY_ith_BLOCK == 0)
                cout << " - " << "access time: " << duration.count() << " ns." << endl;

            // save measured data to the output csv file
            csv_os << i << xmreg::timestamp_to_str(blk.timestamp)
                   << duration.count() << blk_size
                   << core_storage.get_block_id_by_height(i)
                   << blk.tx_hashes.size()
                   << cryptonote::print_money(mcore.get_block_reward(blk))
                   << core_storage.get_db().get_block_difficulty(i)
                   << NEWLINE;
        }
        catch (const exception& e)
        {
            cerr << e.what() << endl;
            continue;
        }


    } // for (uint64_t i = 0; i < height; ++i)


    // colose the output csv file
    csv_os.flush();
    csv_os.close();

    cout << "\nCsv saved as: " << out_csv_file << endl;

    cout << "\nEnd of program." << endl;

    return 0;
}
