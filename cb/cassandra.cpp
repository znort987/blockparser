
// Dump all block rewards

#include <util.h>
#include <common.h>
#include <errlog.h>
#include <option.h>
#include <string.h>
#include <callback.h>
#include <ctime>

#include <boost/lexical_cast.hpp>

#include <boost/asio.hpp>
#include <cql/cql.hpp>
#include <cql/cql_connection.hpp>
#include <cql/cql_session.hpp>
#include <cql/cql_cluster.hpp>
#include <cql/cql_builder.hpp>
#include <cql/cql_result.hpp>

struct CassandraSync:public Callback
{
    optparse::OptionParser parser;

    bool proofOfStake;
    bool emptyOutput;
    uint64_t inputValue;
    uint64_t baseReward;
    uint8_t txCount;
    size_t nbInputs;
    bool hasGenInput;
    uint64_t currBlock;
    uint64_t blockFee;
    uint32_t bits;
    time_t time;
    const uint8_t *currTXHash;

    std::string hostname;
    unsigned short port;
    std::string username;
    std::string password;
    std::string keyspace;

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
            .set_default("localhost")
            .help("hostname of cassandra machine")
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
    }

    virtual const char                   *name() const         { return "peerstats"; }
    virtual const optparse::OptionParser *optionParser() const { return &parser;   }
    virtual bool                         needTXHash() const    { return true;      }

    virtual void aliases(
        std::vector<const char*> &v
    ) const
    {
        v.push_back("sync");
    }
 


    virtual int init(
        int argc,
        const char *argv[]
    )
    {
        optparse::Values values = parser.parse_args(argc, argv);
        hostname = values["hostname"].c_str();
        port = boost::lexical_cast<unsigned short>(values["port"]);
        keyspace = values["keyspace"].c_str();
        
        username = values["username"].c_str();
        password = values["password"].c_str();

        info("initializing connections with cassandra instance as %s for database %s at %s:%h",username.c_str(),keyspace.c_str(),hostname.c_str(),port);


        return 0;
    }

    virtual void startBlock(
        const Block *b,
        uint64_t
    )
    {
        const uint8_t *p = b->data;
        SKIP(uint32_t, version, p);
        SKIP(uint256_t, prevBlkHash, p);
        SKIP(uint256_t, blkMerkleRoot, p);
        LOAD(uint32_t, blkTime, p);
        LOAD(uint32_t, blkBits, p);
        currBlock = b->height - 1;
        bits = blkBits;
        time = blkTime;
        proofOfStake = false;
        baseReward = 0;
        inputValue = 0;
        txCount = 0;
        blockFee = 0;
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
            blockFee += value;
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
        } 
        

    }

    virtual void endBlock(
        const Block *b
    )
    {
        if(!proofOfStake) {
            // use diff(bits) to get the difficulty
        } else {
            uint64_t stakeEarned = baseReward - inputValue;
            //printf("stake earned %f\n",1e-6*stakeEarned);
            // use diff(bits) to get the difficulty
        }
        
    }

    virtual void wrapup() {
    }
};

static CassandraSync cassandra;

