//
// Created by mwo on 5/11/15.
//

#include "MicroCore.h"

namespace xmreg
{
    /**
     * The constructor is interesting, as
     * m_mempool and m_blockchain_storage depend
     * on each other.
     *
     * So basically m_mempool is initialized with
     * reference to Blockchain (i.e., Blockchain&)
     * and m_blockchain_storage is initialized with
     * reference to m_mempool (i.e., tx_memory_pool&)
     *
     * The same is done in cryptonode::core.
     */
    MicroCore::MicroCore():
            m_mempool(m_blockchain_storage),
            m_blockchain_storage(m_mempool)
    {}


    /**
     * Initialized the MicroCore object.
     *
     * Create BlockchainLMDB on the heap.
     * Open database files located in blockchain_path.
     * Initialize m_blockchain_storage with the BlockchainLMDB object.
     */
    bool
    MicroCore::init(const string& blockchain_path)
    {
        int db_flags = 0;

        // MDB_RDONLY will result in
        // m_blockchain_storage.deinit() producing
        // error messages.

        //db_flags |= MDB_RDONLY ;

        db_flags |= MDB_NOSYNC;

        BlockchainDB* db = nullptr;
        db = new BlockchainLMDB();

        try
        {
            // try opening lmdb database files
            db->open(blockchain_path, db_flags);
        }
        catch (const std::exception& e)
        {
            cerr << "Error opening database: " << e.what();
            return false;
        }

        // check if the blockchain database
        // is successful opened
        if(!db->is_open())
        {
            return false;
        }

        // initialize Blockchain object to manage
        // the database.
        return m_blockchain_storage.init(db, false);
    }

    /**
    * Get m_blockchain_storage.
    * Initialize m_blockchain_storage with the BlockchainLMDB object.
    */
    Blockchain&
    MicroCore::get_core()
    {
        return m_blockchain_storage;
    }

    /**
     * Get block by its height
     *
     * returns true if success
     */
    bool
    MicroCore::get_block_by_height(const uint64_t& height, block& blk)
    {

        crypto::hash block_id;

        try
        {
            block_id = m_blockchain_storage.get_block_id_by_height(height);
        }
        catch (const exception& e)
        {
            cerr << e.what() << endl;
            return false;
        }


        if (!m_blockchain_storage.get_block_by_hash(block_id, blk))
        {
            cerr << "Block with hash " << block_id
                 << "and height " << height << " not found!"
                 << endl;
            return false;
        }

        return true;
    }



    /**
     * Get transaction tx from the blockchain using it hash
     */
    bool
    MicroCore::get_tx(const crypto::hash& tx_hash, transaction& tx)
    {
        try
        {
            // get transaction with given hash
            tx = m_blockchain_storage.get_db().get_tx(tx_hash);
        }
        catch (const exception& e)
        {
            cerr << e.what() << endl;
            return false;
        }

        return true;
    }



    /**
     * Returns miner award for solving given block
     */
    uint64_t
    MicroCore::get_block_reward(const block& blk)
    {
        uint64_t reward {0};

        for (const tx_out& out: blk.miner_tx.vout)
        {
            reward += out.amount;
        }

        return reward;
    }




/**
     * De-initialized Blockchain.
     *
     * Its needed to mainly deallocate
     * new BlockchainDB object
     * created in the MicroCore::init().
     *
     * It also tries to synchronize the blockchain.
     * And this is the reason when, if MDB_RDONLY
     * is set, we are getting error messages. Because
     * blockchain is readonly and we try to synchronize it.
     */
    MicroCore::~MicroCore()
    {
        m_blockchain_storage.deinit();
    }
}