
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
    const char *name,
    bool printList
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

    if(1==found.size() && !printList) {
        return found.begin()->second;
    } else if(0==found.size()) {
        printf("\n");
        warning("\"%s\": unknown command name\n", name);
    } else {

        printf("\n");

        if(!printList) {
            warning("\"%s\": ambiguous command name, could be any of:\n", name);
        }

        int count = 0;
        auto e = found.end();
        auto i = found.begin();
        while(i!=e) {

            std::vector<const char*> names;
            Callback *c = (i++)->second;
            c->aliases(names);

            printf(
                "      %3d. %s%s",
                ++count,
                c->name(),
                0<names.size() ? " (aliases: " : ""
            );

            bool first = true;
            auto e = names.end();
            auto i = names.begin();
            while(i!=e) {
                const char *nm = *(i++);
                printf(
                    "%s%s",
                    first ? "" : ", ",
                    nm
                );
                first = false;
            }
            printf(
                "%s\n",
                0<names.size() ? ")" : ""
            );
        }
        printf("\n");
    }

    if(printList || 0==strcmp(name, "help")) return 0;
    printf("use:\n");
    printf("      \"parser man\" for complete documentation of all commands.\n");
    printf("      \"parser help\" for a short summary of available commands.\n\n");
    exit(-1);
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

void Callback::showAllHelps(
    bool longHelp
)
{
    auto e = callbacks->end();
    auto i = callbacks->begin();
    std::sort(i, e, Cmp());

    while(i!=e) {
        Callback *c = *(i++);
        printf("    USAGE: parser %s ", c->name());
        const optparse::OptionParser *parser = c->optionParser();
        if(parser) {
            printf(
                "%s\n",
                longHelp                      ?
                parser->get_usage().c_str()   :
                parser->format_help().c_str()
            );
        }
    }
    printf("\n");
}

