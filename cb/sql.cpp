
// Full SQL dump of the blockchain

#include <util.h>
#include <stdio.h>
#include <common.h>
#include <errlog.h>
#include <option.h>
#include <callback.h>

static uint8_t empty[kSHA256ByteSize] = { 0x42 };

typedef GoogMap<
    Hash256,
    uint64_t,
    Hash256Hasher,
    Hash256Equal
>::Map OutputMap;

struct SQLDump : public Callback {

    FILE *txFile;
    FILE *blockFile;
    FILE *inputFile;
    FILE *outputFile;

    uint64_t txID;
    uint64_t blkID;
    uint64_t inputID;
    uint64_t outputID;
    int64_t cutoffBlock;
    OutputMap outputMap;
    optparse::OptionParser parser;

    SQLDump() {
        parser
            .usage("[options] [list of addresses to restrict output to]")
            .version("")
            .description("create an SQL dump of the blockchain")
            .epilog("")
        ;
        parser
            .add_option("-a", "--atBlock")
            .action("store")
            .type("int")
            .set_default(-1)
            .help("stop dump at block <block> (default: all)")
        ;
    }

    virtual const char                   *name() const         { return "sqldump"; }
    virtual const optparse::OptionParser *optionParser() const { return &parser;   }
    virtual bool                       needUpstream() const    { return true;      }

    virtual void aliases(
        std::vector<const char*> &v
    ) const {
        v.push_back("dump");
    }

    virtual int init(
        int argc,
        const char *argv[]
    ) {
        txID = -1;
        blkID = 0;
        inputID = 0;
        outputID = 0;

        static uint64_t sz = 32 * 1000 * 1000;
        outputMap.setEmptyKey(empty);
        outputMap.resize(sz);

        optparse::Values &values = parser.parse_args(argc, argv);
        cutoffBlock = values.get("atBlock").asInt64();

        info("dumping the blockchain ...");

        txFile = fopen("transactions.txt", "w");
        if(!txFile) sysErrFatal("couldn't open file txs.txt for writing\n");

        blockFile = fopen("blocks.txt", "w");
        if(!blockFile) sysErrFatal("couldn't open file blocks.txt for writing\n");

        inputFile = fopen("inputs.txt", "w");
        if(!inputFile) sysErrFatal("couldn't open file inputs.txt for writing\n");

        outputFile = fopen("outputs.txt", "w");
        if(!outputFile) sysErrFatal("couldn't open file outputs.txt for writing\n");

        FILE *sqlFile = fopen("blockChain.sql", "w");
        if(!sqlFile) sysErrFatal("couldn't open file blockChain.sql for writing\n");

        fprintf(
            sqlFile,
            "\n"
            "DROP DATABASE IF EXISTS blockChain;\n"
            "CREATE DATABASE blockChain;\n"
            "USE blockChain;\n"
            "\n"
            "DROP TABLE IF EXISTS transactions;\n"
            "DROP TABLE IF EXISTS outputs;\n"
            "DROP TABLE IF EXISTS inputs;\n"
            "DROP TABLE IF EXISTS blocks;\n"
            "\n"
            "CREATE TABLE blocks(\n"
            "    id BIGINT PRIMARY KEY,\n"
            "    hash BINARY(32),\n"
            "    time BIGINT\n"
            ");\n"
            "\n"
            "CREATE TABLE transactions(\n"
            "    id BIGINT PRIMARY KEY,\n"
            "    hash BINARY(32),\n"
            "    blockID BIGINT\n"
            ");\n"
            "\n"
            "CREATE TABLE outputs(\n"
            "    id BIGINT PRIMARY KEY,\n"
            "    dstAddress CHAR(36),\n"
            "    value BIGINT,\n"
            "    txID BIGINT,\n"
            "    offset INT\n"
            ");\n"
            "\n"
            "CREATE TABLE inputs(\n"
            "    id BIGINT PRIMARY KEY,\n"
            "    outputID BIGINT,\n"
            "    txID BIGINT,\n"
            "    offset INT\n"
            ");\n"
            "\n"
        );
        fclose(sqlFile);

        FILE *bashFile = fopen("blockChain.bash", "w");
        if(!bashFile) sysErrFatal("couldn't open file blockChain.bash for writing\n");

        fprintf(
            bashFile,
            "\n"
            "#!/bin/bash\n"
            "\n"
            "echo\n"
            "\n"
            "echo 'wiping/re-creating DB blockChain ...'\n"
            "time mysql -u root -p -hlocalhost --password='' < blockChain.sql\n"
            "echo done\n"
            "echo\n"
            "\n"
            "for i in blocks inputs outputs transactions\n"
            "do\n"
            "    echo Importing table $i ...\n"
            "    time mysqlimport -u root -p -hlocalhost --password='' --lock-tables --use-threads=3 --local blockChain $i.txt\n"
            "    echo done\n"
            "    echo\n"
            "done\n"
            "\n"
        );
        fclose(bashFile);

        return 0;
    }

    virtual void startBlock(
        const Block *b,
        uint64_t
    ) {
        if(0<=cutoffBlock && cutoffBlock<b->height) {
            wrapup();
        }

        auto p = b->chunk->getData();
        uint8_t blockHash[kSHA256ByteSize];
        sha256Twice(blockHash, p, 80);

        SKIP(uint32_t, version, p);
        SKIP(uint256_t, prevBlkHash, p);
        SKIP(uint256_t, blkMerkleRoot, p);
        LOAD(uint32_t, blkTime, p);

        // id BIGINT PRIMARY KEY
        // hash BINARY(32)
        // time BIGINT
        fprintf(blockFile, "%" PRIu64 "\t", (blkID = b->height-1));

        writeEscapedBinaryBufferRev(blockFile, blockHash, kSHA256ByteSize);
        fputc('\t', blockFile);

        fprintf(blockFile, "%" PRIu64 "\n", (uint64_t)blkTime);
        if(0==(b->height)%500) {
            fprintf(
                stderr,
                "block=%8" PRIu64 " "
                "nbOutputs=%8" PRIu64 "\n",
                b->height,
                outputMap.size()
            );
        }
    }

    virtual void startTX(
        const uint8_t *p,
        const uint8_t *hash
    ) {
        // id BIGINT PRIMARY KEY
        // hash BINARY(32)
        // blockID BIGINT
        fprintf(txFile, "%" PRIu64 "\t", ++txID);

        writeEscapedBinaryBufferRev(txFile, hash, kSHA256ByteSize);
        fputc('\t', txFile);

        fprintf(txFile, "%" PRIu64 "\n", blkID);
    }

    virtual void endOutput(
        const uint8_t *p,
        uint64_t      value,
        const uint8_t *txHash,
        uint64_t      outputIndex,
        const uint8_t *outputScript,
        uint64_t      outputScriptSize
    ) {
        uint8_t address[40];
        address[0] = 'X';
        address[1] = 0;

        uint8_t addrType[3];
        uint160_t pubKeyHash;
        int type = solveOutputScript(
            pubKeyHash.v,
            outputScript,
            outputScriptSize,
            addrType
        );
        if(likely(0<=type)) {
            hash160ToAddr(
                address,
                pubKeyHash.v,
                false,
                addrType[0]
            );
        }

        // id BIGINT PRIMARY KEY
        // dstAddress CHAR(36)
        // value BIGINT
        // txID BIGINT
        // offset INT
        fprintf(
            outputFile,
            "%" PRIu64 "\t"
            "%s\t"
            "%" PRIu64 "\t"
            "%" PRIu64 "\t"
            "%" PRIu32 "\n"
            ,
            outputID,
            address,
            value,
            txID,
            (uint32_t)outputIndex
        );

        uint32_t oi = outputIndex;
        uint8_t *h = allocHash256();
        memcpy(h, txHash, kSHA256ByteSize);

        uintptr_t ih = reinterpret_cast<uintptr_t>(h);
        uint32_t *h32 = reinterpret_cast<uint32_t*>(ih);
        h32[0] ^= oi;

        outputMap[h] = outputID++;
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
        uint256_t h;
        uint32_t oi = outputIndex;
        memcpy(h.v, upTXHash, kSHA256ByteSize);

        uintptr_t ih = reinterpret_cast<uintptr_t>(h.v);
        uint32_t *h32 = reinterpret_cast<uint32_t*>(ih);
        h32[0] ^= oi;

        auto src = outputMap.find(h.v);
        if(outputMap.end()==src) {
            errFatal("unconnected input");
        }

        // id BIGINT PRIMARY KEY
        // outputID BIGINT
        // txID BIGINT
        // offset INT
        fprintf(
            inputFile,
            "%" PRIu64 "\t"
            "%" PRIu64 "\t"
            "%" PRIu64 "\t"
            "%" PRIu32 "\n"
            ,
            inputID++,
            src->second,
            txID,
            (uint32_t)inputIndex
        );
    }

    virtual void wrapup() {
        fclose(outputFile);
        fclose(inputFile);
        fclose(blockFile);
        fclose(txFile);
        info("done\n");
    }
};

static SQLDump sqlDump;

