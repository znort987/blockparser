
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

typedef const uint8_t *Hash160;
struct uint160_t { uint8_t v[kRIPEMD160ByteSize]; };

typedef uint160_t Addr;
template<> uint8_t *PagedAllocator<Addr>::pool = 0;
template<> uint8_t *PagedAllocator<Addr>::poolEnd = 0;
static inline Addr *allocAddr() { return (Addr*)PagedAllocator<Addr>::alloc(); }

struct Hash160Hasher
{
    uint64_t operator()(
        const Hash160 &hash160
    ) const
    {
        uintptr_t i = reinterpret_cast<uintptr_t>(hash160);
        const uint64_t *p = reinterpret_cast<const uint64_t*>(i);
        return p[0];
    }
};

struct Hash160Equal
{
    bool operator()(
        const Hash160 &ha,
        const Hash160 &hb
    ) const
    {
        uintptr_t ia = reinterpret_cast<uintptr_t>(ha);
        uintptr_t ib = reinterpret_cast<uintptr_t>(hb);

        const uint64_t *a0 = reinterpret_cast<const uint64_t *>(ia);
        const uint64_t *b0 = reinterpret_cast<const uint64_t *>(ib);
        if(unlikely(a0[0]!=b0[0])) return false;
        if(unlikely(a0[1]!=b0[1])) return false;

        const uint32_t *a1 = reinterpret_cast<const uint32_t *>(ia);
        const uint32_t *b1 = reinterpret_cast<const uint32_t *>(ib);
        if(unlikely(a1[4]!=b1[4])) return false;

        return true;
    }
};

typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::undirectedS> Graph;
typedef GoogMap<Hash160, uint64_t, Hash160Hasher, Hash160Equal >::Map AddrMap;
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
        if(type<0)
            return;

        uint64_t a;
        auto i = gAddrMap.find(pubKeyHash.v);
        if(gAddrMap.end()!=i)
            a = i->second;
        else {
            Addr *addr = allocAddr();
            memcpy(addr->v, pubKeyHash.v, kRIPEMD160ByteSize);
            gAddrMap[addr->v] = a = gAllAddrs.size();
            gAllAddrs.push_back(addr);
        }

        vertices.push_back(a);
    }

    Closure()
    {
        Callback::add("closure", this);
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
        printf("done, %.2f secs.\n\n", usecs() - startTime);

        size_t size = boost::num_vertices(gGraph);
        printf("Clustering %d addresses ... \n", (int)size);
        startTime = usecs();

        std::vector<uint64_t> cc(size);
        size_t nbCC = boost::connected_components(gGraph, &cc[0]);
        printf(
            "done, %.2f secs, %d clusters.\n\n",
            usecs() - startTime,
            (int)nbCC
        );

        const uint8_t *keyHash = loadKeyHash();
        printf("Showing address cluster for address 0x");
        showHex(keyHash, sizeof(uint160_t), false);
        printf("\n");

        auto i = gAddrMap.find(keyHash);
        if(gAddrMap.end()==i)
            errFatal("specified key was not found");

        uint64_t addrIndex = i->second;
        uint64_t homeComponentIndex = cc[addrIndex];
        for(size_t i=0; i<cc.size(); ++i) {
            uint64_t componentIndex = cc[i];
            if(homeComponentIndex==componentIndex) {
                Addr *addr = gAllAddrs[i];
                printf("    0x");
                showHex(addr->v, sizeof(uint160_t), false);
                printf("\n");
            }
        }
    }

    virtual void startTX(
        const uint8_t *p
    )
    {
        vertices.resize(0);
    }

    virtual void endTX(
        const uint8_t *p
    )
    {
        size_t size = vertices.size();
        if(1<size) {
            for(size_t i=1; i<size; ++i) {
                uint64_t a = vertices[i-1];
                uint64_t b = vertices[i-0];
                boost::add_edge(a, b, gGraph);
            }
        }
    }

    virtual void   startBlock(const uint8_t *p) {}
    virtual void     endBlock(const uint8_t *p) {}
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

static Closure closure;

