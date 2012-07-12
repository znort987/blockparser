
#include <vector>
#include <stdio.h>
#include <string.h>
#include <common.h>
#include <algorithm>
#include <callback.h>
static std::vector<Callback*> *callbacks;

Callback::Callback()
{
    if(0==callbacks) callbacks = new std::vector<Callback*>;
    callbacks->push_back(this);
}

Callback *Callback::find(
    const char *name
)
{
    auto e = callbacks->end();
    auto i = callbacks->begin();
    while(i!=e) {
        Callback *c = *(i++);
        const char *n = c->name();
        if(0==strcmp(name, n)) return c;
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
    auto e = callbacks->end();
    auto i = callbacks->begin();
    std::sort(i, e, Cmp());

    while(i!=e) {
        Callback *c = *(i++);
        const option::Descriptor *usage = c->usage();

        if(usage) {
            Writer writer;
            printf("\n");
            option::printUsage(&writer, usage);
        }
    }
    printf("\n");
}

