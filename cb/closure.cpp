
// Dump the transitive closure of a bunch of addresses

#include <util.h>
#include <timer.h>
#include <common.h>
#include <errlog.h>
#include <option.h>
#include <rmd160.h>
#include <callback.h>

#include <vector>
#include <string.h>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/connected_components.hpp>

typedef uint160_t Addr;
static uint8_t gEmptyKey[kRIPEMD160ByteSize] = { 0x52 };

typedef GoogMap<
    Hash160,
    uint64_t,
    Hash160Hasher,
    Hash160Equal
>::Map AddrMap;

typedef boost::adjacency_list<
    boost::vecS,
    boost::vecS,
    boost::undirectedS
> Graph;

struct Closure : public Callback {

    optparse::OptionParser parser;

    Graph graph;
    AddrMap addrMap;
    double startTime;
    std::vector<Addr*> allAddrs;
    std::vector<uint64_t> vertices;
    std::vector<uint160_t> rootHashes;

    Closure() {
        parser
            .usage("[list of addresses to seed the closure]")
            .version("")
            .description("builds a list of addresses provably controlled by the same party.")
            .epilog("")
        ;
    }

    virtual const char                   *name() const         { return "closure"; }
    virtual const optparse::OptionParser *optionParser() const { return &parser;   }
    virtual bool                       needUpstream() const    { return true;      }

    virtual void aliases(
        std::vector<const char*> &v
    ) const {
        v.push_back("cluster");
        v.push_back("wallet");
    }

    virtual int init(
        int argc,
        const char *argv[]
    ) {
        optparse::Values &values = parser.parse_args(argc, argv);

        auto args = parser.args();
        for(size_t i=1; i<args.size(); ++i) {
            loadKeyList(rootHashes, args[i].c_str());
        }

        if(0==rootHashes.size()) {
            const char *addr = getInterestingAddr();
            loadKeyList(rootHashes, addr);
        }

        addrMap.setEmptyKey(gEmptyKey);
        addrMap.resize(15 * 1000 * 1000);
        allAddrs.reserve(15 * 1000 * 1000);
        info("Building address equivalence graph ...");
        startTime = Timer::usecs();
        return 0;
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
        uint8_t addrType[3];
        uint160_t pubKeyHash;
        int type = solveOutputScript(pubKeyHash.v, outputScript, outputScriptSize, addrType);
        if(unlikely(type<0)) return;

        uint64_t a;
        auto i = addrMap.find(pubKeyHash.v);
        if(unlikely(addrMap.end()!=i))
            a = i->second;
        else {
            Addr *addr = (Addr*)allocHash160();
            memcpy(addr->v, pubKeyHash.v, kRIPEMD160ByteSize);
            addrMap[addr->v] = a = allAddrs.size();
            allAddrs.push_back(addr);
        }

        vertices.push_back(a);
    }

    virtual void wrapup() {
        size_t size = boost::num_vertices(graph);
        info(
            "done, %.2f secs, found %" PRIu64 " address(es) \n",
            1e-6*(Timer::usecs() - startTime),
            size
        );

        info("Clustering ... ");
        startTime = Timer::usecs();

        std::vector<uint64_t> cc(size);
        uint64_t nbCC = boost::connected_components(graph, &cc[0]);
        info(
            "done, %.2f secs, found %" PRIu64 " clusters.\n",
            1e-6*(Timer::usecs() - startTime),
            nbCC
        );

        auto e = rootHashes.end();
        auto i = rootHashes.begin();
        while(e!=i) {

            uint64_t count = 0;
            const uint8_t *keyHash = (i++)->v;

            uint8_t b58[128];
            hash160ToAddr(b58, keyHash);
            info("Address cluster for address %s:", b58);

            auto j = addrMap.find(keyHash);
            if(unlikely(addrMap.end()==j)) {
                warning("specified key was never used to spend coins");
                showFullAddr(keyHash);
                printf("\n");
                count = 1;
            } else {
                uint64_t addrIndex = j->second;
                uint64_t homeComponentIndex = cc[addrIndex];
                for(size_t k=0; likely(k<cc.size()); ++k) {
                    uint64_t componentIndex = cc[k];
                    if(unlikely(homeComponentIndex==componentIndex)) {
                        Addr *addr = allAddrs[k];
                        showFullAddr(addr->v);
                        printf("\n");
                        ++count;
                    }
                }
            }
            info("%" PRIu64 " addresse(s)\n", count);
        }
    }

    virtual void startTX(
        const uint8_t *p,
        const uint8_t *
    ) {
        vertices.resize(0);
    }

    virtual void endTX(
        const uint8_t *p
    ) {
        size_t size = vertices.size();
        if(likely(1<size)) {
            for(size_t i=1; unlikely(i<size); ++i) {
                uint64_t a = vertices[i-1];
                uint64_t b = vertices[i-0];
                boost::add_edge(a, b, graph);
            }
        }
    }
};

static Closure closure;

