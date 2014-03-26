
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
    uint128_t totalSent;
    uint128_t totalReceived;

    PeerStats()
    {
        parser
            .usage("")
            .version("")
            .description("peercoin block chain statistics")
            .epilog("")
        ;
        parser
            .add_option("-b", "--blocks")
            .action("store_true")
            .set_default(false)
            .help("analyze the last recent x blocks")
        ;
    }

    virtual const char                   *name() const         { return "peerstats"; }
    virtual const optparse::OptionParser *optionParser() const { return &parser;   }
    virtual bool                         needTXHash() const    { return true;      }

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

        info("parsing high level peercoin stats");
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
        if((proofOfStake && txCount == 2) || hasGenInput) {
            baseReward += value;
        } else {
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
            totalTrans += txCount - 1;
            // use diff(bits) to get the difficulty
        } else {
            POScount++;
            uint64_t stakeEarned = baseReward - inputValue;
            //printf("stake earned %f\n",1e-6*stakeEarned);
            totalStakeEarned += stakeEarned;
            totalStaked += inputValue;
            totalTrans += txCount - 2;
            // use diff(bits) to get the difficulty
        }
        totalFeeDestroyed += blockFee;
        
    }

    virtual void wrapup() {

    uint64_t totalSupply = totalMined + totalStakeEarned - totalFeeDestroyed;
    #define P(x) (pr128(x).c_str())
        printf("\n");
        printf(" Total Blocks = %s\n",P(POWcount+POScount));
        printf(" POS Blocks   = %s\n",P(POScount));
        printf(" POW Blocks   = %s\n",P(POWcount));
        //these aren't work right just yet
        //inputValue is not being populated
        printf("\n");
        printf(" Total Coins Destroyed = %12.6f\n",1e-6*totalFeeDestroyed);
        printf(" Total Coins Mined = %16.6f\n",1e-6*totalMined);
        printf(" POS Coins Minted  = %16.6f\n",1e-6*totalStakeEarned);
        printf(" Total Coin Supply = %16.6f\n",1e-6*totalSupply);
        printf("\n");
        printf(" Total Coins used in Stake Generation = %16.6f\n",1e-6*totalStaked);
        printf("\n");
        printf(" Total Transactions = %s\n",P(totalTrans));
        printf(" Total Sent     = %16.6f\n",1e-6*totalSent);
        printf(" Total Received = %16.6f\n",1e-6*totalReceived);
        printf("\n");
    #undef P
    }
};

static PeerStats peerstats;

