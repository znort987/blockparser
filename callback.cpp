
#include <map>
#include <vector>
#include <stdio.h>
#include <string.h>
#include <algorithm>

#include <util.h>
#include <common.h>
#include <errlog.h>
#include <callback.h>

static std::vector<Callback*> *callbacks;
typedef std::map<uintptr_t, Callback*> CBMap;

Callback::Callback() {
    if(0==callbacks) {
        callbacks = new std::vector<Callback*>;
    }
    callbacks->push_back(this);
}

Callback *Callback::find(
    const char *name,
    bool printList
) {

    CBMap found;
    auto sz = strlen(name);
    for(auto c:*callbacks) {

        std::vector<const char*> names;
        names.push_back(c->name());
        c->aliases(names);

        for(const auto nm:names) {
            auto x = std::min(sz, strlen(nm));
            if(0==strncasecmp(nm, name, x)) {
                found[(uintptr_t)c] = c;
            }
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
            warning(
                "\"%s\": ambiguous command name, could be any of:\n",
                name
            );
        }

        int count = 0;
        for(auto pair:found) {

            std::vector<const char*> names;
            auto c = pair.second;
            c->aliases(names);

            printf(
                "      %3d. %s%s",
                ++count,
                c->name(),
                0<names.size() ? " (aliases: " : ""
            );

            auto first = true;
            for(const auto nm:names) {
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

    if(printList || 0==strcmp(name, "help")) {
        return 0;
    }

    printf("use:\n");
    printf("      \"parser man\" for complete documentation of all commands.\n");
    printf("      \"parser help\" for a short summary of available commands.\n\n");
    exit(-1);
}

static void printHelp(
    const Callback *c,
    bool longHelp
) {
    printf("\n--------------------------------------------------------------------------------------\n");
    printf("USAGE:\n\n    parser %s ", c->name());
    const auto parser = c->optionParser();
    if(parser) {
        auto usg = parser->usage();
        printf("%s\n\n", usg.c_str());

        auto opt = parser->format_option_help();
        if(opt.size()) {
            printf("OPTIONS:\n\n%s\n", opt.c_str());
        }
    }
}

void Callback::showHelpFor(
    const char *name,
    bool longHelp
) {
    auto cb = find(name);
    if(cb) {
        printHelp(cb, longHelp);
    }
}

struct Cmp {
    bool operator()(
        const Callback *a,
        const Callback *b
    ) {
        auto an = a->name();
        auto bn = b->name();
        auto n = strcmp(an, bn);
        return n<0;
    }
};

void Callback::showAllHelps(
    bool longHelp
) {
    std::sort(
        callbacks->begin(),
        callbacks->end(),
        Cmp()
    );
    for(const auto c:*callbacks) {
        printHelp(c, longHelp);
    }
    printf("\n");
}

