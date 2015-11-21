
// Dump everything known about a TX

#include <util.h>
#include <common.h>
#include <errlog.h>
#include <string.h>
#include <callback.h>

typedef GoogMap<
    Hash256,
    int,
    Hash256Hasher,
    Hash256Equal
>::Map TxMap;

struct DumpTX:public Callback {

    optparse::OptionParser parser;

    bool dump;
    bool isDone;
    TxMap txMap;
    bool isGenTX;
    uint64_t bTime;
    uint64_t valueIn;
    uint64_t valueOut;
    uint64_t nbInputs;
    uint64_t nbOutputs;
    uint64_t currBlock;
    uint32_t txVersion;
    uint64_t nbDumped;
    const uint8_t *txStart;
    std::vector<uint256_t> rootHashes;

    DumpTX() {
        parser
            .usage("[list of transaction hashes]")
            .version("")
            .description(
                "dumpp all the details of  the list of specified transactions"
            )
            .epilog("")
        ;
    }

    virtual const char                   *name() const         { return "dumpTX"; }
    virtual const optparse::OptionParser *optionParser() const { return &parser;  }
    virtual bool                       needUpstream() const    { return true;     }
    virtual bool                               done()          { return isDone;   }

    virtual void aliases(
        std::vector<const char*> &v
    ) const {
        v.push_back("txinfo");
        v.push_back("txshow");
        v.push_back("showtx");
        v.push_back("txdetails");
    }

    virtual int init(
        int        argc,
        const char *argv[]
    ) {

        nbDumped = 0;
        isDone = false;

        optparse::Values &values = parser.parse_args(argc, argv);

        auto args = parser.args();
        for(size_t i=1; i<args.size(); ++i) {
            loadHash256List(rootHashes, args[i].c_str());
        }

        if(0<rootHashes.size()) {
            info("dumping %d transactions\n", (int)rootHashes.size());
        } else {
            const char *defaultTX = "a1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d"; // Expensive pizza
            warning("no TX hashes specified, using the famous 10K pizza TX");
            loadHash256List(rootHashes, defaultTX);
        }

        static uint8_t empty[kSHA256ByteSize] = { 0x42 };
        txMap.setEmptyKey(empty);

        for(auto const &txHash : rootHashes) {
            txMap[txHash.v] = 1;
        }

        return 0;
    }

    virtual void startBlock(
        const Block *b,
        uint64_t
    ) {
        currBlock = b->height;

        const uint8_t *p = b->chunk->getData();
        SKIP(uint32_t, version, p);
        SKIP(uint256_t, prevBlkHash, p);
        SKIP(uint256_t, blkMerkleRoot, p);
        LOAD(uint32_t, blkTime, p);
        bTime = blkTime;
    }

    virtual void startTX(
        const uint8_t *p,
        const uint8_t *hash
    ) {
        #if defined(CLAM)
            auto pBis = p;
            LOAD(uint32_t, nVersion, pBis);
            txVersion = nVersion;
        #endif

        txStart = p;
        nbInputs = 0;
        nbOutputs = 0;
        dump = (txMap.end()!=txMap.find(hash));

        if(dump) {

            struct tm gmTime;
            time_t blockTime = bTime;
            gmtime_r(&blockTime, &gmTime);

            char timeBuf[256];
            asctime_r(&gmTime, timeBuf);

            size_t sz =strlen(timeBuf);
            if(0<sz) timeBuf[sz-1] = 0;

            LOAD(uint32_t, version, p);

            fprintf(stderr, "                                                 \n");
            printf("TX = {\n\n");
            printf("    version = %" PRIu32 "\n", version);
            printf("    minted in block = %" PRIu64 "\n", currBlock-1);
            printf("    mint time = %" PRIu64 " (%s GMT)\n", bTime, timeBuf);
            printf("    txHash = ");
            showHex(hash);
            printf("\n\n");
        }
    }

    virtual void startInputs(
        const uint8_t *p
    ) {
    }

    virtual void endInputs(
        const uint8_t *p
    ) {
    }

    virtual void startInput(
        const uint8_t *p
    ) {
        if(dump) {
            printf(
                "    input[%" PRIu64 "] = {\n\n",
                nbInputs++
            );

            static uint256_t gNullHash;
            LOAD(uint256_t, upTXHash, p);
            LOAD(uint32_t, upOutputIndex, p);
            LOAD_VARINT(inputScriptSize, p);
            showScript(p, inputScriptSize, 0, "        ");

            isGenTX = (0==memcmp(gNullHash.v, upTXHash.v, sizeof(gNullHash)));
            if(isGenTX) {
                uint64_t reward = getBaseReward(currBlock);
                printf("        generation transaction\n");
                printf("        based on block height, reward = %.8f\n", satoshisToNormaForm(reward));
                printf("        hex dump of coinbase follows:\n\n");
                canonicalHexDump(p, inputScriptSize, "        ");
                valueIn += reward;
            }
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
        uint64_t      inputScriptSize
    ) {
        if(dump) {
            uint8_t buf[1 + 2*kSHA256ByteSize];
            toHex(buf, upTXHash);
            printf("        outputIndex = %" PRIu64 "\n", outputIndex);
            printf("        value = %.8f\n", satoshisToNormaForm(value));
            printf("        upTXHash = %s\n\n", buf);
            printf("        # challenge answer script, bytes=%" PRIu64 " (on downstream input) =\n", inputScriptSize);
            showScript(inputScript, inputScriptSize, 0, "        ");
            printf("                           ||\n");
            printf("                           VV\n");
            printf("        # challenge script, bytes=%" PRIu64 " (on upstream output)=\n", outputScriptSize);
            showScript(outputScript, outputScriptSize, 0, "        ");
            showScriptInfo(outputScript, outputScriptSize, (const uint8_t *)"        ");
            valueIn += value;

        }
    }

    virtual void endInput(
        const uint8_t *p
    ) {
        if(dump) {
            printf("    }\n\n");
        }
    }

    virtual void startOutputs(
        const uint8_t *p
    ) {
    }

    virtual void endOutputs(
        const uint8_t *p
    ) {
        #if defined(CLAM)
            if(1<txVersion) {
                LOAD_VARINT(strCLAMSpeechLen, p);
                printf("    comment = '\n");
                    canonicalHexDump(
                        p,
                        strCLAMSpeechLen,
                        "    "
                    );
                printf("'\n");
            }
        #endif
    }

    virtual void startOutput(
        const uint8_t *p
    ) {
        if(dump) {
            printf(
                "\n"
                "    output[%" PRIu64 "] = {\n\n",
                nbOutputs++
            );
        }
    }

    virtual void endOutput(
        const uint8_t *p,                   // Pointer to TX output raw data
        uint64_t      value,                // Number of satoshis on this output
        const uint8_t *txHash,              // sha256 of the current transaction
        uint64_t      outputIndex,          // Index of this output in the current transaction
        const uint8_t *outputScript,        // Raw script (challenge to would-be spender) carried by this output
        uint64_t      outputScriptSize      // Byte size of raw script
    ) {
        if(dump) {
            printf("        value = %.8f\n", satoshisToNormaForm(value));
            printf("        challenge script, bytes=%" PRIu64 " :\n", outputScriptSize);
            showScript(outputScript, outputScriptSize, 0, "        ");
            showScriptInfo(outputScript, outputScriptSize, (const uint8_t *)"        ");
            printf("    }\n\n");
            valueOut += value;
        }
    }

    virtual void endTX(
        const uint8_t *p
    ) {
        if(dump) {
            LOAD(uint32_t, lockTime, p);
            printf("    nbInputs = %" PRIu64 "\n", (uint64_t)nbInputs);
            printf("   nbOutputs = %" PRIu64 "\n", (uint64_t)nbOutputs);
            printf("    byteSize = %" PRIu64 "\n", (uint64_t)(p - txStart));
            printf("    lockTime = %" PRIu32 "\n", (uint32_t)lockTime);
            printf("     valueIn =  %.2f\n", satoshisToNormaForm(valueIn));
            printf("    valueOut =  %.2f\n", satoshisToNormaForm(valueOut));
            if(!isGenTX) {
                printf("        fees =  %.2f\n", satoshisToNormaForm(valueIn-valueOut));
            }
            printf("}\n");
            ++nbDumped;
        }
        isDone = (nbDumped==txMap.size());
    }
};

static DumpTX dumpTX;

