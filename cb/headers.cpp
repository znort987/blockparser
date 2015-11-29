
// Dump block headers

#include <util.h>
#include <common.h>
#include <option.h>
#include <callback.h>

struct Headers : public Callback {

    optparse::OptionParser parser;

    Headers() {
        parser
            .usage("")
            .version("")
            .description("Dump all bock headers")
            .epilog("")
        ;
    }

    virtual const char                   *name() const         { return "headers"; }
    virtual const optparse::OptionParser *optionParser() const { return &parser;   }
    virtual bool                       needUpstream() const    { return false;     }

    virtual void startBlock(
        const Block *b,
        uint64_t chainSize
    ) {
        uint8_t hash[2*kSHA256ByteSize + 1];
        toHex(hash, b->hash);

        uint8_t prevHash[2*kSHA256ByteSize + 1];
        memset(prevHash, '0', sizeof(prevHash));
        prevHash[2*kSHA256ByteSize] = 0;
        if(b->prev) {
            toHex(prevHash, b->prev->hash);
        }

        uint8_t nextHash[2*kSHA256ByteSize + 1];
        memset(nextHash, '0', sizeof(nextHash));
        nextHash[2*kSHA256ByteSize] = 0;
        if(b->next) {
            toHex(nextHash, b->next->hash);
        }

        printf(
            "%s %8" PRIu64 " prev=(%s %8" PRIu64 " ) next=(%s %8" PRIu64 " )\n",
            hash,
            b->height,
            prevHash,
            (b->prev ? b->prev->height :0),
            nextHash,
            (b->next ? b->next->height :0)
        );
    }
};

static Headers headers;

