
#include <map>
#include <util.h>
#include <vector>
#include <stdio.h>
#include <string.h>
#include <common.h>
#include <errlog.h>
#include <algorithm>
#include <callback.h>
static std::vector<Callback*> *callbacks;
typedef std::map<uintptr_t, Callback*> CBMap;

Callback::Callback()
{
    if(0==callbacks) callbacks = new std::vector<Callback*>;
    callbacks->push_back(this);
}

Callback *Callback::find(
    const char *name
)
{
    CBMap found;
    size_t sz = strlen(name);
    auto e = callbacks->end();
    auto i = callbacks->begin();
    while(i!=e) {
        Callback *c = *(i++);

        std::vector<const char*> names;
        names.push_back(c->name());
        c->aliases(names);

        auto e = names.end();
        auto i = names.begin();
        while(i!=e) {
            const char *nm = *(i++);
            size_t x = std::min(sz, strlen(nm));
            if(0==strncasecmp(nm, name, x)) found[(uintptr_t)c] = c;
        }
    }
    if(1==found.size()) return found.begin()->second;
    if(1<found.size()) {
        printf("\n");
        warning("\"%s\": ambiguous command name, could be any of:\n", name);

        int count = 0;
        auto e = found.end();
        auto i = found.begin();
        while(i!=e) {
            Callback *c = (i++)->second;
            printf(" %3d. %s", ++count, c->name());

            std::vector<const char*> names;
            c->aliases(names);

            auto e = names.end();
            auto i = names.begin();
            while(i!=e) {
                const char *nm = *(i++);
                printf(", %s", nm);
            }
            printf("\n");
        }
        printf("\n");
    }
    return 0;
}

struct Cmp
{
    bool operator()(
        const Callback *a,
        const Callback *b
    )
    {
        const char *an = a->name();
        const char *bn = b->name();
        int n = strcmp(an, bn);
        return n<0;
    }
};

struct Writer {
    void operator()(
        const char *buf,
        size_t size
    )
    {
        fwrite(buf, size, 1, stdout);
    }
}; 

void Callback::showAllHelps()
{
    Writer writer;
    auto e = callbacks->end();
    auto i = callbacks->begin();
    std::sort(i, e, Cmp());

    while(i!=e) {
        Callback *c = *(i++);
        printf("    parser %s", c->name());
        const option::Descriptor *usage = c->usage();

        if(usage) {
            option::printUsage(&writer, usage);
        }
        printf("\n");
    }
    printf("\n");
}

