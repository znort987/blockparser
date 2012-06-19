
// Dump all transactions affecting a specific address

#include <util.h>
#include <common.h>
#include <rmd160.h>
#include <string.h>
#include <callback.h>

struct Transactions:public Callback
{
    uint64_t sum;
    uint64_t nbTX;

    virtual bool needTXHash()
    {
        return true;
    }

    virtual int init(
        int argc,
        char *argv[]
    )
    {
        bool ok = (0==argc || 1==argc);
        if(!ok) return -1;

        loadKeyHash((const uint8_t*)argv[0]);
        return 0;
    }

    void move(
        const uint8_t *script,
        uint64_t      scriptSize,
        const uint8_t *txHash,
        uint64_t       value,
        bool           add,
        const uint8_t *downTXHash = 0
    )
    {
        uint8_t addrType[3];
        uint8_t pubKeyHash[kRIPEMD160ByteSize];
        int type = solveOutputScript(pubKeyHash, script, scriptSize, addrType);
        if(type<0)
            return;

        const uint8_t *targetKeyHash = loadKeyHash();
        if(0==memcmp(targetKeyHash, pubKeyHash, sizeof(pubKeyHash))) {

            printf("    ");
            showHex(downTXHash ? downTXHash : txHash);

            int64_t newSum = sum + value*(add ? 1 : -1);
            printf(
                " %24.08f %c %24.08f = %24.08f\n",
                sum*1e-8,
                add ? '+' : '-',
                value*1e-8,
                newSum*1e-8
            );
            sum = newSum;
            ++nbTX;
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
        move(
            outputScript,
            outputScriptSize,
            txHash,
            value,
            true
        );
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
        uint64_t      inputScriptSize
    )
    {
        move(
            outputScript,
            outputScriptSize,
            upTXHash,
            value,
            false,
            downTXHash
        );
    }

    Transactions()
    {
        sum = 0;
        Callback::add("transactions", this);
    }

    virtual void startMap(
        const uint8_t *p
    )
    {

        printf("Dumping all transactions for address ");
        showHex(loadKeyHash(), kRIPEMD160ByteSize, false);
        printf("\n\n");
        printf("    Transaction                                                                    OldBalance                     Amount                 NewBalance\n");
        printf("    ===============================================================================================================================================\n");
    }

    virtual void endMap(
        const uint8_t *p
    )
    {
        printf(
            "    ===============================================================================================================================================\n"
            "\n"
            "    %" PRIu64 " transactions, final balance = %.08f\n"
            "\n",
            nbTX,
            sum*1e-8
        );
    }

    virtual void   startBlock(const uint8_t *p) {}
    virtual void     endBlock(const uint8_t *p) {}
    virtual void      startTX(const uint8_t *p) {}
    virtual void        endTX(const uint8_t *p) {}
    virtual void  startInputs(const uint8_t *p) {}
    virtual void    endInputs(const uint8_t *p) {}
    virtual void   startInput(const uint8_t *p) {}
    virtual void     endInput(const uint8_t *p) {}
    virtual void startOutputs(const uint8_t *p) {}
    virtual void   endOutputs(const uint8_t *p) {}
    virtual void  startOutput(const uint8_t *p) {}
    virtual void   startBlock(  const Block *b) {}
    virtual void     endBlock(  const Block *b) {}
};

static Transactions transactions;

