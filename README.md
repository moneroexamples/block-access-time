# Measure block access time
While working on another example, I've noticed that access time to some
monero blocks, especially early ones, using `Blockchain::get_block_by_hash`,
is very long. Thus, I wanted check this for all blocks  blockchain.

As a result of this, this example was created showing how to loop through all
blocks in lmdb blockchain, and save some basic information from the blocks,
as well as measure access time, into an output csv file.

## Pre-requisites

Everything here was done and tested on
Ubuntu 14.04 x86_64 and Ubuntu 15.10 x86_64.

Monero source code compilation and setup are same as [here](http://moneroexamples.github.io/access-blockchain-in-cpp/).


## C++: main.cpp

```c++
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
```

## Results

- The resulted csv file can be seen [here](https://mega.nz/#!P4cWWTLK!Eb5m4q4f5Tx-5p5FNMwF7cv0ckvPTX5Hy5fqGn7VFm4).

- The plot of log of block access time against block number is [here](http://i.imgur.com/2xmAF0c.png).

From the csv and data, it can be seen that there is very large
concentration of very long times up to block of about 100k. This
results in very slow recovery of your deterministic wallet, when
the blocks in this range are being scanned for your transactions.


## Compile this example
The dependencies are same as those for Monero, so I assume Monero compiles
correctly. If so then to download and compile this example, the following
steps can be executed:

```bash
# download the source code
git clone https://github.com/moneroexamples/block-access-time

# enter the downloaded sourced code folder
cd block-access-time

# create the makefile
cmake .

# compile
make
```

After this, `blkaccesstime` executable file should be present in access-blockchain-in-cpp
folder. How to use it, can be seen in the above example outputs.


## How can you help?

Constructive criticism, code and website edits are always good. They can be made through github.

Some Monero are also welcome:
```
48daf1rG3hE1Txapcsxh6WXNe9MLNKtu7W7tKTivtSoVLHErYzvdcpea2nSTgGkz66RFP4GKVAsTV14v6G3oddBTHfxP6tU
```
