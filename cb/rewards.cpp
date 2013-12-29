
// Dump all block rewards

#include <util.h>
#include <common.h>
#include <errlog.h>
#include <option.h>
#include <string.h>
#include <callback.h>
#include <ctime>

struct Rewards:public Callback
{
    optparse::OptionParser parser;

    bool fullDump;
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

    Rewards()
    {
        parser
            .usage("")
            .version("")
            .description("dump all block rewards")
            .epilog("")
        ;
        parser
            .add_option("-f", "--full")
            .action("store_true")
            .set_default(false)
            .help("dump fee transaction details")
        ;
    }

    virtual const char                   *name() const         { return "rewards"; }
    virtual const optparse::OptionParser *optionParser() const { return &parser;   }
    virtual bool                         needTXHash() const    { return true;      }

    virtual int init(
        int argc,
        const char *argv[]
    )
    {
        optparse::Values &values = parser.parse_args(argc, argv);
        fullDump = values.get("full");

        info("Dumping all block rewards in blockchain");
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
            //printf("adding %f from tx:%d fee:%f\n",1e-6*value,txCount,1e-6*blockFee);
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
            //printf("subtracting %f from tx:%d fee:%f\n",1e-6*value,txCount,1e-6*blockFee);
        } 

        uint8_t addrType[3];
        uint160_t pubKeyHash;
        int type = solveOutputScript(
            pubKeyHash.v,
            outputScript,
            outputScriptSize,
            addrType
        );
        if(unlikely(-2==type)) return;

        if(unlikely(type<0) && 0!=value && fullDump) {
            printf("============================\n");
            printf("BLOCK %d ... RAW ASCII DUMP OF FAILING SCRIPT = ", (int)currBlock);
            fwrite(outputScript, outputScriptSize, 1, stdout);
            printf("value = %16.8f\n", value*1e-6);
            showScript(outputScript, outputScriptSize);
            printf("============================\n\n");
            printf("\n");
            errFatal("invalid script");
        }

        if(!fullDump) return;

        printf("%7d ", (int)currBlock);
        showHex(currTXHash);

        printf(" %16.8f ", 1e-6*value);

        if(type<0) {
            printf("######################################## ##################################\n");
            return;
        } else {

            showFullAddr(pubKeyHash.v, true);
            printf(" %2d ", type);

            // pay to hash160(pubKey)
            if(0==type) {
                // No pubkey
            }

            // pay to explicit pubKey
            if(1==type) {
                showHex(1+outputScript, 65, false);
            }

            // pay to explicit compressed pubKeys
            if(2==type) {
                uint8_t decompressed[65];
                bool r = decompressPublicKey(decompressed, 1+outputScript);
                showHex(decompressed, 65, false);
            }

            // pay to hash160(script)
            if(3==type) {
                // No pubkey, no script
            }

            printf("\n");
        }
    }

    virtual void endBlock(
        const Block *b
    )
    {
        const char *blockType = (proofOfStake) ? "POS" : "POW";
        if(!proofOfStake) {
            printf(
                "Summary for block %6d @ %s : type=%s diff=%.2f                  mined      =%12.6f destroyedfees=%8.6f\n",
                (int)currBlock,
                gettime(time),
                blockType,
                diff(bits),
                1e-6*baseReward,
                1e-6*blockFee
            );
        } else {
            int64_t stakeEarned = baseReward - inputValue;
            printf(
                "Summary for block %6d @ %s : type=%s diff=%.4f staked=%14.6f stakeEarned=%12.6f destroyedfees=%8.6f\n",
                (int)currBlock,
                gettime(time),
                blockType,
                diff(bits),
                1e-6*inputValue,
                1e-6*stakeEarned,
                1e-6*blockFee
            );            
        }
    }
};

static Rewards rewards;

