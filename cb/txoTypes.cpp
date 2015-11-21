
// Catalog of all TXO types in the block chain

#include <util.h>
#include <common.h>
#include <errlog.h>
#include <string.h>
#include <callback.h>

struct TXOTypes:public Callback {

    bool seen[10];
    uint64_t currBlock;
    const uint8_t *txHash;
    optparse::OptionParser parser;

    TXOTypes() {
        parser
            .usage("")
            .version("")
            .description(
                "dump a catalog of all TX output types encountered in the block chain"
            )
            .epilog("")
        ;
    }

    virtual const char                   *name() const         { return "txotype"; }
    virtual const optparse::OptionParser *optionParser() const { return &parser;   }
    virtual bool                       needUpstream() const    { return true;      }

    virtual void aliases(
        std::vector<const char*> &v
    ) const {
    }

    virtual int init(
        int        argc,
        const char *argv[]
    ) {
        memset(seen, 0, sizeof(seen)/sizeof(seen[0]));
        currBlock = -1;
        txHash = 0;
        return 0;
    }

    virtual void startBlock(
        const Block *b,
        uint64_t
    ) {
        currBlock = b->height;
    }

    virtual void startTX(
        const uint8_t *p,
        const uint8_t *hash
    ) {
        txHash = hash;
    }

    virtual void endOutput(
        const uint8_t *p,                   // Pointer to TX output raw data
        uint64_t      value,                // Number of satoshis on this output
        const uint8_t *txHash,              // sha256 of the current transaction
        uint64_t      outputIndex,          // Index of this output in the current transaction
        const uint8_t *outputScript,        // Raw script (challenge to would-be spender) carried by this output
        uint64_t      outputScriptSize      // Byte size of raw script
    ) {
        uint160_t hash;
        uint8_t addrType[3];
        auto r = solveOutputScript(
            hash.v,
            outputScript,
            outputScriptSize,
            addrType
        );

        auto doPrint = true;
        auto isCommon = (0<=r && r<4);
        if(isCommon) {
            doPrint = (false==seen[r]);
            seen[r] = true;
        }

        if(doPrint) {
            printf("--------------------------------------------\n");
            printf(
                "Found %sscript of type %d\n",
                isCommon ? "first instance of " : "",
                r
            );
            printf("appears in:\n");
            printf("    block  %d\n", (int)currBlock);
            printf("    tx     ");
            showHex(txHash);
            printf("\n");
            printf("    output %d\n", (int)outputIndex);
            printf("\n");
            printf(
                "    value: %.2f BTC (%" PRIu64 " satoshis)\n",
                satoshisToNormaForm(value),
                value
            );
            printf("\n");
            printf("    script info:\n");
            showScriptInfo(outputScript, outputScriptSize, (const uint8_t *)"        ");
            printf("\n");
            printf("    script dump:\n");
            showScript(outputScript, outputScriptSize, 0, "        ", true);
            printf("--------------------------------------------\n");
        }
    }
};

static TXOTypes txoTypes;

