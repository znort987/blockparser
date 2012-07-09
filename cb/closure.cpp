
// Dump the transitive closure of all addresses

#include <util.h>
#include <common.h>
#include <errlog.h>
#include <rmd160.h>
#include <callback.h>

#include <vector>
#include <string.h>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/connected_components.hpp>

typedef uint160_t Addr;
typedef GoogMap<Hash160, uint64_t, Hash160Hasher, Hash160Equal >::Map AddrMap;
typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::undirectedS> Graph;

static uint8_t gEmptyKey[kRIPEMD160ByteSize] = { 0x52 };
static std::vector<Addr*> gAllAddrs;
static AddrMap gAddrMap;
static Graph gGraph;

struct Closure:public Callback
{
    double startTime;
    std::vector<uint64_t> vertices;

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

    virtual void endOutput(
        const uint8_t *p,
        uint64_t      value,
        const uint8_t *txHash,
        uint64_t      outputIndex,
        const uint8_t *outputScript,
        uint64_t      outputScriptSize
    )
    {
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
        uint8_t addrType[3];
        uint160_t pubKeyHash;
        int type = solveOutputScript(pubKeyHash.v, outputScript, outputScriptSize, addrType);
        if(unlikely(type<0))
            return;

        uint64_t a;
        auto i = gAddrMap.find(pubKeyHash.v);
        if(unlikely(gAddrMap.end()!=i))
            a = i->second;
        else {
            Addr *addr = (Addr*)allocHash160();
            memcpy(addr->v, pubKeyHash.v, kRIPEMD160ByteSize);
            gAddrMap[addr->v] = a = gAllAddrs.size();
            gAllAddrs.push_back(addr);
        }

        vertices.push_back(a);
    }

    virtual void startMap(
        const uint8_t *p
    )
    {
        gAddrMap.setEmptyKey(gEmptyKey);
        gAddrMap.resize(15 * 1000 * 1000);
        gAllAddrs.reserve(15 * 1000 * 1000);
        printf("Building address equivalence graph ...\n");
        startTime = usecs();
    }

    virtual void endMap(
        const uint8_t *p
    )
    {
        printf("done, %.2f secs.\n\n", 1e-6*(usecs() - startTime));

        size_t size = boost::num_vertices(gGraph);
        printf("Clustering %d addresses ... \n", (int)size);
        startTime = usecs();

        std::vector<uint64_t> cc(size);
        size_t nbCC = boost::connected_components(gGraph, &cc[0]);
        printf(
            "done, %.2f secs, found %d clusters.\n\n",
            1e-6*(usecs() - startTime),
            (int)nbCC
        );

        const uint8_t *keyHash = loadKeyHash();

        printf("Address cluster for address ");
        showHex(keyHash, sizeof(uint160_t), false);
        printf(":\n");

        auto i = gAddrMap.find(keyHash);
        if(unlikely(gAddrMap.end()==i))
            warning("specified key was not found");

        uint64_t addrIndex = i->second;
        uint64_t homeComponentIndex = cc[addrIndex];
        for(size_t i=0; likely(i<cc.size()); ++i) {
            uint64_t componentIndex = cc[i];
            if(unlikely(homeComponentIndex==componentIndex)) {

                Addr *addr = gAllAddrs[i];

                uint8_t b58[128];
                hash160ToAddr(b58, addr->v);
                printf("%s ", b58);

                showHex(addr->v, sizeof(uint160_t), false);
                printf("\n");

            }
        }
    }

    virtual void startTX(
        const uint8_t *p,
        const uint8_t *
    )
    {
        vertices.resize(0);
    }

    virtual void endTX(
        const uint8_t *p
    )
    {
        size_t size = vertices.size();
        if(likely(1<size)) {
            for(size_t i=1; i<size; ++i) {
                uint64_t a = vertices[i-1];
                uint64_t b = vertices[i-0];
                boost::add_edge(a, b, gGraph);
            }
        }
    }

    virtual const char *name()
    {
        return "closure";
    }
};

static Closure closure;

