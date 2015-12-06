
// Full dump of the blockchain

#include <util.h>
#include <string.h>
#include <callback.h>

struct RawDump:public Callback {

    uint64_t txId;
    bool isCoinBase;
    uint32_t inputId;
    uint32_t outputId;
    uint32_t currBlock;
    uint32_t txVersion;
    uint32_t txIdInBlock;
    uint32_t indentLevel;
    uint8_t spaces[1024];
    optparse::OptionParser parser;

    RawDump() {

        txId = 0;
        currBlock = 0;
        indentLevel = 0;
        memset(spaces, ' ' , sizeof(spaces));
        spaces[0] = 0;

        parser
            .usage("")
            .version("")
            .description("full dump of the block chain")
            .epilog("")
        ;
    }

    virtual const char                   *name() const         { return "rawdump";  }
    virtual const optparse::OptionParser *optionParser() const { return &parser;    }
    virtual bool                       needUpstream() const    { return true;       }

    void push() {
        spaces[4*indentLevel] = ' ';
            if(4*(1+indentLevel)<sizeof(spaces)) {
                ++indentLevel;
            }
        spaces[4*indentLevel] = 0;
    }

    void pop() {
        spaces[4*indentLevel] = ' ';
            if(0<indentLevel) {
                --indentLevel;
            }
        spaces[4*indentLevel] = 0;
    }

    // Called when the second parse of the full chain starts
    virtual void start(
        const Block *s,
        const Block *e
    ) {
        printf("%schain = {\n", spaces);
        push();
    }

    // Called when a new block is encountered
    virtual void startBlock(
        const Block *b,
        uint64_t    chainSize
    ) {
        txIdInBlock = 0;
        currBlock = b->height;

        printf(
            "%sblock%d = {\n",
            spaces,
            (int)(-1+b->height)
        );
        push();

        printf(
            "%ssize = %" PRIu64 "\n",
            spaces,
            b->chunk->getSize()
        );

        printf(
            "%soffset = %" PRIu64 "\n",
            spaces,
            b->chunk->getOffset()
        );

        printf(
            "%smap = '%s'\n",
            spaces,
            b->chunk->getBlockFile()->name.c_str()
        );

        printf("%sblockHash = '", spaces);
        showHex(b->hash);
        printf("'\n");
    }

    // Called when start list of TX is encountered
    virtual void startTXs(
        const uint8_t *p
    ) {
        printf("%stransactions = {\n", spaces);
        push();
    }

    // Called when a new TX is encountered
    virtual void startTX(
        const uint8_t *p,
        const uint8_t *hash
    ) {

        printf(
            "%stx%d = {\n",
            spaces,
            txIdInBlock
        );
        push();

        #if defined(CLAM)
            auto pBis = p;
            LOAD(uint32_t, nVersion, pBis);
            txVersion = nVersion;
        #endif

        printf("%stxHash = '", spaces);
        showHex(hash);
        printf("'\n");
    }

    // Called when the start of a TX's input array is encountered
    virtual void startInputs(
        const uint8_t *p
    ) {
        inputId = 0;
        isCoinBase = false;
        printf("%sinputs = {\n", spaces);
        push();
    }

    // Called when a TX input is encountered
    virtual void startInput(
        const uint8_t *p
    ) {
        printf(
            "%sinput%d = {\n",
            spaces,
            (int)inputId
        );
        push();

        static uint256_t gNullHash;
        LOAD(uint256_t, upTXHash, p);
        LOAD(uint32_t, upOutputIndex, p);
        LOAD_VARINT(inputScriptSize, p);

        printf("%sscript = '\n", spaces);
            pop();
                showScript(p, inputScriptSize, 0, (const char *)spaces);
            push();
        printf("%s'\n", spaces);

        isCoinBase = (0==memcmp(gNullHash.v, upTXHash.v, sizeof(gNullHash)));
        if(isCoinBase) {
            uint64_t value = getBaseReward(currBlock);
            printf("%sisCoinBase = true\n", spaces);
            printf(
                "%svalue = %" PRIu64 " # %.08f\n",
                spaces,
                value,
                satoshisToNormaForm(value)
            );
            printf("%scoinBase = '\n", spaces);
            push();
                canonicalHexDump(
                    p,
                    inputScriptSize,
                    (const char *)spaces
                );
            pop();
            printf("%s'\n", spaces);
        }
    }

    // Called exactly like startInput, but with a much richer context
    virtual void edge(
        uint64_t      value,                // Number of satoshis coming in on this input from upstream transaction
        const uint8_t *upTXHash,            // sha256 of upstream transaction
        uint64_t      outputIndex,          // Index of output in upstream transaction
        const uint8_t *outputScript,        // Raw script (challenge to spender) carried by output in upstream transaction
        uint64_t      outputScriptSize,     // Byte size of script carried by output in upstream transaction
        const uint8_t *downTXHash,          // sha256 of current (downstream) transaction
        uint64_t      inputIndex,           // Index of input in downstream transaction
        const uint8_t *inputScript,         // Raw script (answer to challenge) carried by input in downstream transaction
        uint64_t      inputScriptSize       // Byte size of script carried by input in downstream transaction
    )
    {
        printf(
            "%svalue = %" PRIu64 " # %.08f\n",
            spaces,
            value,
            satoshisToNormaForm(value)
        );

        printf(
            "%ssourceTXOutputIndex = %d\n",
            spaces,
            (int)outputIndex
        );

        printf(
            "%ssourceTXHash = '",
            spaces
        );
        showHex(upTXHash);
        printf("'\n");
    }

    // Called when at the end of a TX input
    virtual void endInput(
        const uint8_t *p
    ) {
        if(!isCoinBase) {
            printf("%sisCoinBase = false\n", spaces);
        }

        pop();
        printf("%s}\n", spaces);
        ++inputId;
    }

    // Called when the end of a TX's input array is encountered
    virtual void endInputs(
        const uint8_t *p
    ) {
        pop();
        printf("%s}\n", spaces);
    }

    // Called when the start of a TX's output array is encountered
    virtual void startOutputs(
        const uint8_t *p
    ) {
        outputId = 0;
        printf("%soutputs = {\n", spaces);
        push();
    }

    // Called when a TX output is encountered
    virtual void startOutput(
        const uint8_t *p
    ) {
        printf(
            "%soutput%d = {\n",
            spaces,
            (int)outputId
        );
        push();
    }

    // Called when an output has been fully parsed
    virtual void endOutput(
        const uint8_t *p,                   // Pointer to TX output raw data
        uint64_t      value,                // Number of satoshis on this output
        const uint8_t *txHash,              // sha256 of the current transaction
        uint64_t      outputIndex,          // Index of this output in the current transaction
        const uint8_t *outputScript,        // Raw script (challenge to would-be spender) carried by this output
        uint64_t      outputScriptSize      // Byte size of raw script
    )
    {
        printf(
            "%svalue = %" PRIu64 " # %.08f\n",
            spaces,
            value,
            satoshisToNormaForm(value)
        );

        printf("%sscript = '\n", spaces);
        showScript(outputScript, outputScriptSize, 0, (const char *)spaces);
        printf("%s'\n", spaces);

        showScriptInfo(outputScript, outputScriptSize, spaces);

        pop();
        printf("%s}\n", spaces);
        ++outputId;
    }

    // Called when the end of a TX's output array is encountered
    virtual void endOutputs(
        const uint8_t *p
    ) {
        #if defined(CLAM)
            if(1<txVersion) {
                LOAD_VARINT(strCLAMSpeechLen, p);
                printf("%stxComment = '\n", spaces);
                    push();
                        canonicalHexDump(
                            p,
                            strCLAMSpeechLen,
                            (const char *)spaces
                        );
                    pop();
                printf("%s'\n", spaces);
            }
        #endif

        pop();
        printf("%s}\n", spaces);
    }

    // Called when an end of TX is encountered
    virtual void endTX(
        const uint8_t *p
    ) {
        pop();
        printf("%s}\n", spaces);
        ++txIdInBlock;
        ++txId;
    }

    // Called when end list of TX is encountered
    virtual void endTXs(
        const uint8_t *p
    ) {
        pop();
        printf("%s}\n", spaces);
    }

    // Called when an end of block is encountered
    virtual void endBlock(
        const Block *b
    ) {
        pop();
        printf("%s}\n", spaces);
    }

    virtual void      startLC(                                     )       {               }  // Called when longest chain parse starts
    virtual void       wrapup(                                     )       {               }  // Called when the whole chain has been parsed

};

static RawDump rawDump;

