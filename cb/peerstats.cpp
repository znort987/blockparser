
// Dump all block rewards

#include <util.h>
#include <common.h>
#include <errlog.h>
#include <option.h>
#include <string.h>
#include <callback.h>
#include <ctime>

struct PeerStats:public Callback
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

    uint32_t POScount;
    uint32_t POWcount;
    uint64_t totalFeeDestroyed;
    uint64_t totalMined;
    uint64_t totalStakeEarned;
    uint64_t totalStaked;
    uint32_t totalTrans;
    uint64_t totalTransferred;
    uint64_t totalSent;
    uint64_t totalReceived;

    PeerStats()
    {
        parser
            .usage("")
            .version("")
            .description("peercoin block chain statistics")
            .epilog("")
        ;
        parser
            .add_option("-f", "--full")
            .action("store_true")
            .set_default(false)
            .help("dump fee transaction details")
        ;
    }

    virtual const char                   *name() const         { return "peerstats"; }
    virtual const optparse::OptionParser *optionParser() const { return &parser;   }

    virtual void aliases(
        std::vector<const char*> &v
    ) const
    {
        v.push_back("stats");
    }
 


    virtual int init(
        int argc,
        const char *argv[]
    )
    {
        optparse::Values &values = parser.parse_args(argc, argv);
        //fullDump = values.get("full");

        info("Dumping all block rewards in blockchain");
        POScount = 0;
        POWcount = 0;
        totalFeeDestroyed = 0;
        totalMined = 0;
        totalStakeEarned = 0;
        totalStaked = 0;
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
        SKIP(uint256_t, prevBlkHash, p);
        SKIP(uint256_t, blkMerkleRoot, p);
        LOAD(uint32_t, blkTime, p);
        LOAD(uint32_t, blkBits, p);
        currBlock = b->height - 1;
        reward = 0;
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

        if(hasGenInput) {
            baseReward += value;
        } else if(proofOfStake && txCount == 2) {
            inputValue += value;
        } else {
            totalTrans++;
            totalSent += value;
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
        if(proofOfStake && txCount == 2) {
            baseReward += value;
        } else if(!hasGenInput) {
            blockFee -= value;
            totalReceived += value;
        } 
        

    }

    virtual void endBlock(
        const Block *b
    )
    {
        if(!proofOfStake) {
            POWcount++;
            totalMined += baseReward;
            // use diff(bits) to get the difficulty
        } else {
            POScount++;
            int64_t stakeEarned = baseReward - inputValue;
            totalStakeEarned += stakeEarned;
            totalStaked += inputValue;
            // use diff(bits) to get the difficulty
        }
        totalFeeDestroyed += blockFee;
        
    }
};

static PeerStats peerstats;

