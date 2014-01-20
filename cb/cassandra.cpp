
// Dump all block rewards

#include <util.h>
#include <common.h>
#include <errlog.h>
#include <option.h>
#include <string.h>
#include <callback.h>
#include <ctime>

#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string/predicate.hpp>

#include <boost/asio.hpp>
#include <cql/cql.hpp>
#include <cql/cql_connection.hpp>
#include <cql/cql_session.hpp>
#include <cql/cql_cluster.hpp>
#include <cql/cql_builder.hpp>
#include <cql/cql_result.hpp>
    
using namespace cql;
using boost::shared_ptr;
    
void
print_rows(
    cql::cql_result_t& result)
{
    while (result.next()) {
        for (size_t i = 0; i < result.column_count(); ++i) {
            cql::cql_byte_t* data = NULL;
            cql::cql_int_t size = 0;

            result.get_data(i, &data, size);

            std::cout.write(reinterpret_cast<char*>(data), size);
            for (int i = size; i < 25; i++) {
                std::cout << ' ' ;
            }
            std::cout << " | ";
        }
        std::cout << std::endl;
    }
}

void log_callback(
        const cql::cql_short_t,
        const std::string& message)
    {
        std::cout << "LOG: " << message << std::endl;
    }
long from_counter(const std::string& value) {
    union  {
        long    v;
        char    cc[8];
    } xx;
    for (size_t i=0; i < 8;++i) {
        xx.cc[7-i]=value.at(i);
    }
    return xx.v;
}
struct CassandraSync:public Callback
{
    optparse::OptionParser parser;


    bool proofOfStake;
    bool verbose;
    bool emptyOutput;
    bool skip;
    bool drop;
    uint64_t inputValue;
    uint64_t baseReward;
    uint8_t txCount;
    int blkTxCount;
    size_t nbInputs;
    bool hasGenInput;
    uint64_t currBlock;
    uint64_t blockFee;
    uint32_t bits;
    uint32_t nonce;
    const uint8_t *prevBlkHash;
    const uint8_t *blkMerkleRoot;
    time_t time;
    const uint8_t *currTXHash;
    
    uint32_t POScount;
    uint32_t POWcount;
    uint64_t totalFeeDestroyed;
    uint64_t totalMined;
    uint64_t totalStakeEarned;
    uint32_t totalTrans;
    uint128_t totalSent;
    uint128_t totalReceived;
    uint64_t block_sent;
    uint64_t block_received;

    int block_inserts;
    int block_existing;
    int64_t database_block_count;

    std::string hostname;
    unsigned short port;
    std::string username;
    std::string password;
    std::string keyspace;
        
    shared_ptr<cql::cql_cluster_t> cluster;
    shared_ptr<cql::cql_session_t> session; 
    boost::shared_future<cql::cql_future_result_t> future;
    shared_ptr<cql::cql_builder_t> builder;

    CassandraSync()
    {
        parser
            .usage("[options]")
            .version("")
            .description("load/sync the blockchain with a cassandra database")
            .epilog("")
        ;
        parser
            .add_option("-h", "--host")
            .dest("hostname")
            .set_default("127.0.0.1")
            .help("IP address of cassandra machine")
        ;
        parser
            .add_option("-P", "--port")
            .dest("port")
            .set_default("9042")
            .help("port of cassandra machine")
        ;
        parser
            .add_option("-u", "--user")
            .dest("username")
            .set_default("peeruser")
            .help("user to access cassandra database")
        ;
        parser
            .add_option("-p", "--password")
            .dest("password")
            .set_default("pass")
            .help("password to cassandra database")
        ;
        parser
            .add_option("-k", "--keyspace")
            .dest("keyspace")
            .set_default("peerchain")
            .help("name of keyspace to use")
        ;
        parser
            .add_option("-c", "--cassandra-log")
            .action("store_true")
            .set_default(false)
            .help("print cassandra logs (warning very verbose)")
         ;
        parser
            .add_option("-v", "--verbose")
            .action("store_true")
            .set_default(false)
            .help("verbose")
         ;
        parser
            .add_option("-d", "--drop")
            .action("store_true")
            .set_default(false)
            .help("drop keyspace if it exists")
         ;

    }

    virtual const char                   *name() const         { return "cassandrasync"; }
    virtual const optparse::OptionParser *optionParser() const { return &parser;   }
    virtual bool                         needTXHash() const    { return true;      }

    virtual void aliases(
        std::vector<const char*> &v
    ) const
    {
        v.push_back("sync");
    }

    virtual bool keyspace_exists(std::string keyspace) {
        shared_ptr<cql::cql_query_t> get_keyspaces(new cql::cql_query_t("SELECT * FROM system.schema_keyspaces;"));
        future = session->query(get_keyspaces);
        future.wait();
        if(future.get().error.is_err()) {
            std::cout << boost::format("cql error: %1%") % future.get().error.message << "\n";
            errFatal("Failed to fetch existing keyspaces");
        }
        shared_ptr<cql_result_t> result = future.get().result;
        
        return keyspace_match(keyspace,*future.get().result);
    }

    virtual bool keyspace_match(std::string keyspace,cql::cql_result_t& result) {

        while(result.next()) {
            cql::cql_byte_t* data = NULL;
            cql::cql_int_t size = 0;
            result.get_data("keyspace_name",&data,size);
            char* name = reinterpret_cast<char*>(data); 
            if(verbose) {
                printf("found keyspace %s\n",name);
            }
            if(boost::equals(name,keyspace))
                return true; 

        }    
        return false;

    }

    virtual bool init_cassie(bool cassandra_log) {
        builder = cql::cql_cluster_t::builder();
        if(cassandra_log) {
            builder->with_log_callback(&log_callback);
        }
        builder->add_contact_point(boost::asio::ip::address::from_string(hostname));
        cluster = builder->build();
        session = cluster->connect(); 
        return true; //add an actual check here to see if we're connected
    }

    virtual bool drop_keyspace() {
        return call_query(str(boost::format("DROP KEYSPACE %1%") % keyspace));

    }
    virtual int64_t get_block_count() {
       shared_ptr<cql::cql_query_t> create_keyspace(new cql::cql_query_t(
       str(boost::format("SELECT value from counters where name = 'blocks';"))));
       future = session->query(create_keyspace);
       future.wait();
       if(future.get().error.is_err()) {
          std::cout << boost::format("cql error: %1%") % future.get().error.message << "\n";
          errFatal("Failed to query for block count");
        }
        //print_rows(*future.get().result);
        shared_ptr<cql::cql_result_t> result_ptr = future.get().result;
        cql::cql_result_t& result = *result_ptr;
        if(result.next()) {
            std::string str_counter; 
            //counters need to be fetched as strings and converted
            result.get_string("value",str_counter);
            int64_t count = from_counter(str_counter);
            //info("%lld %"PRIu64" %d", count, count, sizeof(int64_t));
            return count;
            
        } 

        errFatal("Unable to fetch recent block count");
        return 0;

    }
    virtual bool create_keyspace() {
       return call_query(str(boost::format("CREATE KEYSPACE %1% WITH REPLICATION = { 'class' : 'SimpleStrategy', 'replication_factor' : 1 };") % keyspace));
    }
    virtual bool switch_keyspace() {
       return call_query(str(boost::format("USE %1%;") % keyspace));
     }
    virtual bool call_query(std::string query) {
       shared_ptr<cql::cql_query_t> inc(new cql::cql_query_t(query));
       future = session->query(inc);
       future.wait();
       if(future.get().error.is_err()) {
           std::cout << boost::format("cql error: %1%") % future.get().error.message << "\n";
           errFatal("Failed to call query '%s'", query.c_str());
       }
       return true;
    }
    virtual bool increment_counters() {
       call_query("UPDATE counters SET value = value+1 where name = 'blocks';");
       uint64_t totalSupply = totalMined + totalStakeEarned - totalFeeDestroyed;
       uint64_t msTime = time*1000;
       std::string query = str(boost::format("INSERT INTO stats (time, last_block, destroyed_fees,"
        " minted_coins, mined_coins,"
        " money_supply, pos_blocks, pow_blocks, transactions) VALUES "
        "(%d,%d,%d,%d,%d,%d,%d,%d,%d)") % msTime % (int)currBlock % 
        totalFeeDestroyed % totalStakeEarned % totalMined % totalSupply % 
        POScount % POWcount % totalTrans); 
        //totalSent % totalReceived % totalTrans);
       if(verbose) printf("%s\n",query.c_str());
       call_query(query);
       return true;
    }
    virtual bool create_counter_table() {
       return call_query(
       "CREATE TABLE IF NOT EXISTS counters ("
       "  name varchar PRIMARY KEY,"
       "  value counter) with caching = all;"
       );
    }
    virtual bool create_stats_table() {
        return call_query("CREATE TABLE IF NOT EXISTS stats ("
        " time timestamp,"
        " last_block int,"
        " POS_blocks int,"
        " POW_blocks int,"
        " transactions int,"
        " money_supply bigint,"
        " destroyed_fees bigint,"
        " minted_coins bigint,"
        " mined_coins bigint,"
        " PRIMARY KEY (last_block)"
        //" sent bigint,"
        //" received bigint,"
        ") with caching = 'all';"
        );

    }
    virtual bool create_block_table() {
       return call_query(
       "CREATE TABLE IF NOT EXISTS blocks ("
            "id int PRIMARY KEY,"
            "POS boolean,"
            "hashPrevBlock varchar,"
            "hashMerkleRoot varchar,"
            "time timestamp,"
            "bits varchar,"
            "diff float,"
            "nonce bigint,"
            "txcount int,"
            "reward bigint,"
            "staked bigint,"
            "sent bigint,"
            "received bigint,"
            "destroyed bigint)"
            " with caching = 'all';"
       );
    }
    //need to make it parse txns
    virtual bool create_tx_table() {
       return false;
       return call_query(
       "CREATE TABLE IF NOT EXISTS transactions ("
            "id int PRIMARY KEY,"
            "POS boolean,"
            "hashPrevBlock varchar,"
            "hashMerkleRoot varchar,"
            "time timestamp,"
            "bits varchar,"
            "diff float,"
            "nonce bigint,"
            "txcount int,"
            "reward bigint,"
            "staked bigint,"
            "destroyed bigint)"
            " with caching = 'all';"
       );
    }

    virtual bool block_exists() {
       if(drop) {
            return false;
       }
       shared_ptr<cql::cql_query_t> block_exists(new cql::cql_query_t(str(boost::format("SELECT id FROM blocks WHERE ID = %1%;") % (int)currBlock)));
       future = session->query(block_exists);
       future.wait();
       if(future.get().error.is_err()) {
           std::cout << boost::format("cql error: %1%") % future.get().error.message << "\n";
           errFatal("Failed to check if block exists");
       }
       cql::cql_result_t& result = *future.get().result;
       if(result.next()) {
            if(verbose) { info("block %d already exists",(int)currBlock); }
            return true;
       }
       if(verbose) { info("block %d does not exists",(int)currBlock); }
       return false;

        
    }

    virtual void add_block() {

       std::string POS = proofOfStake ? "true" : "false";
       uint8_t *strprevBlkHash = allocHash256(); 
       uint8_t *strblkMerkleRoot = allocHash256();
       toHex(strprevBlkHash,prevBlkHash);
       toHex(strblkMerkleRoot,blkMerkleRoot);
       uint64_t msTime = time*1000;
       std::string query = str(boost::format(
       "INSERT INTO blocks (id,pos,hashprevblock,hashmerkleroot,time,bits,diff,nonce,txcount,reward,staked,sent,received,destroyed) "
       "VALUES (%d,%s,'%s','%s',%d,'%x',%f,%u,%d,%d,%d,%d,%d,%d)") % (int)currBlock % POS % strprevBlkHash % strblkMerkleRoot % msTime % bits %
            diff(bits) % nonce % blkTxCount % baseReward % inputValue % block_sent % block_received % blockFee);
       if(verbose) {
            printf("%s\n",query.c_str());
       }
       shared_ptr<cql::cql_query_t> add_block(new cql::cql_query_t(query));
       future = session->query(add_block);
       future.wait();
       if(future.get().error.is_err()) {
           std::cout << boost::format("cql error: %1%") % future.get().error.message << "\n";
           errFatal("failed to add block %d",(int)currBlock);
       }
       block_inserts++;
    }

    virtual int init(
        int argc,
        const char *argv[]
    )
    {
        block_inserts = 0;
        block_existing = 0;

        optparse::Values values = parser.parse_args(argc, argv);
        hostname = values["hostname"].c_str();
        port = boost::lexical_cast<unsigned short>(values["port"]);
        keyspace = values["keyspace"].c_str();
        
        username = values["username"].c_str();
        password = values["password"].c_str();

        bool cassandra_log = values.get("cassandra_log");
        drop = values.get("drop");
        verbose = values.get("verbose");

        info("connecting to %s@%s:%d in keyspace %s",username.c_str(),hostname.c_str(),port,keyspace.c_str());
        cql_initialize();
        //cql_thread_infrastructure_t cql_ti;

        try {
            if(init_cassie(cassandra_log)) {
                info("connected successfully"); 
            }
            if(drop) {
                if(drop_keyspace()) {
                    info("dropped keyspace %s",keyspace.c_str());
                }
            }
            //print_rows(*future.get().result);
            bool exists = keyspace_exists(keyspace);
            if(exists) {
                info("keyspace %s already exists",keyspace.c_str());
                
            } else {
                info("keyspace %s does not exist",keyspace.c_str());
                //create keyspace
                if(create_keyspace()) {
                   info("created keyspace %s", keyspace.c_str());
                }
                //create block table
            }
            if(switch_keyspace() && verbose) {
                info("switched successfully to keyspace %s",keyspace.c_str());
            }
            if(!exists) {
                //create counter tables
                create_counter_table();
                create_stats_table();
            }
            if(create_block_table() && verbose) {
                info("successfully created/did not delete block table");
            }
            if(create_tx_table() && verbose) {
                info("successfully created/did not delete block table");
            }
            if(exists) {
                database_block_count = get_block_count();
                info("found %lld existing blocks",database_block_count);
            } else {
                database_block_count = 0;
            }
            printf("\n");
            info("starting block insert process");

        }
        catch (std::exception& e)
        {
            std::cout << "Exception: " << e.what() << std::endl;
        }

        
        POScount = 0;
        POWcount = 0;
        totalFeeDestroyed = 0;
        totalMined = 0;
        totalStakeEarned = 0;
        totalTrans = 0;
        totalSent = 0;
        totalReceived = 0;
        return 0;
    }

    virtual void startBlock(
        const Block *b,
        uint64_t
    )
    {
        const uint8_t *p = b->data;
        SKIP(uint32_t, version, p);
        prevBlkHash = p;
        SKIP(uint256_t, prevhash, p);
        blkMerkleRoot = p;
        SKIP(uint256_t, merkleroot, p);
        LOAD(uint32_t, blkTime, p);
        LOAD(uint32_t, blkBits, p);
        LOAD(uint32_t, blkNonce, p);
        LOAD_VARINT(nbTX, p);
        currBlock = b->height - 1;
        bits = blkBits;
        time = blkTime;
        nonce = blkNonce;
        proofOfStake = false;
        baseReward = 0;
        inputValue = 0;
        txCount = 0;
        blkTxCount = nbTX;
        blockFee = 0;
        block_sent = 0;
        block_received = 0;
        //currblock will be 1 less then block_count but < works out
        if((int)currBlock < database_block_count || block_exists()) {
            skip = true;
            block_existing++;
        } else 
            skip = false;
    }

    virtual void startTX(
        const uint8_t *p,
        const uint8_t *hash
    )
    {
        currTXHash = hash;
        txCount++;
        SKIP(uint32_t, version, p);
    }

    virtual void  startInputs(const uint8_t *p)
    {
        hasGenInput = false;
        emptyOutput = false;
        nbInputs = 0;
    }

    virtual void   startInput(const uint8_t *p)
    {
        static uint256_t gNullHash;
        bool isGenInput = (0==memcmp(gNullHash.v, p, sizeof(gNullHash)));
        if(isGenInput) hasGenInput = true;
        ++nbInputs;
    }

    virtual void  endInputs(const uint8_t *p)
    {
        if(hasGenInput) {
            if(1!=nbInputs) abort();
        }
    }
    virtual void edge(
        uint64_t      value,
        const uint8_t *upTXHash,
        uint64_t      outputIndex,
        const uint8_t *outputScript,
        
        uint64_t      outputScriptSize,
        const uint8_t *downTXHash,
        uint64_t      inputIndex,
        const uint8_t *inputScript,
        uint64_t      inputScriptSize) {

        if(proofOfStake && txCount == 2) {
            inputValue += value;
        } else {
            totalSent += value;
            block_sent += value;
            blockFee += value;
            //if(verbose) printf("blockfee %f\n",blockFee*1e-6);
        }
    }

    virtual void endOutput(
        const uint8_t *p,
        uint64_t      value,
        const uint8_t *txHash,
        uint64_t      outputIndex,
        const uint8_t *outputScript,
        uint64_t      outputScriptSize
    )
    {
        if(hasGenInput && outputScriptSize == 0) {
            proofOfStake = true;
        } 
        if((proofOfStake && txCount == 2) || hasGenInput) {
            baseReward += value;
        } else {
            blockFee -= value;
            totalReceived += value;
            block_received += value;
            //if(verbose) printf("blockfee %f\n",blockFee*1e-6);
        } 
        

    }

    virtual void endBlock(
        const Block *b
    )
    {
        if(proofOfStake) {
           //update reward since we know its POS
           int64_t stakeEarned = baseReward - inputValue;
           if(stakeEarned < 0) {
               blockFee -= stakeEarned;
               baseReward = 0;
           } else  
               baseReward = stakeEarned;
           POScount++;
           totalStakeEarned += baseReward;
           if((int)currBlock > 0)
            totalTrans += txCount - 2;
        } else {
            POWcount++;
            totalMined += baseReward;
            totalTrans += txCount - 1;
        }
        totalFeeDestroyed += blockFee;
        if(!skip) {
             add_block();
             increment_counters();
        }
        if(((int)currBlock % 10000) == 0)
            info("processed block %d...%d/%d inserts/existing",(int)currBlock,block_inserts,block_existing);
    }

    virtual void wrapup() {
      printf("\n");
      info("last block processed: %d",(int)currBlock);
      info("inserted blocks: %d",block_inserts);
      info("existing blocks: %d",block_existing);
      info("total blocks processed: %d",block_inserts+block_existing);
      cql_terminate();
      //session->close();
      //cluster->shutdown();
    }
};

static CassandraSync cassandra;

