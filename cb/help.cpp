
// Dump help

#include <stdio.h>
#include <common.h>
#include <errlog.h>
#include <option.h>
#include <callback.h>

struct Help : public Callback {

    optparse::OptionParser parser;

    virtual const char                           *name() const { return "help";  }
    virtual const optparse::OptionParser *optionParser() const { return &parser; }
    virtual bool                          needUpstream() const { return false;   }
    virtual bool                                  done()       { return true;    }

    virtual void aliases(
        std::vector<const char*> &v
    ) const {
        v.push_back("-h");
        v.push_back("-help");
        v.push_back("--help");
        v.push_back("manpage");
        v.push_back("usage");
        v.push_back("doc");
    }

    Help() {
        parser
            .usage("[options]")
            .version("")
            .description("dump help")
            .add_help_option(false)
            .epilog("")
        ;
        parser
            .add_option("-l", "--long")
            .action("store_true")
            .set_default(false)
            .help("produce long and detailed help output")
        ;
    }

    virtual int init(
        int argc,
        const char *argv[]
    ) {
        optparse::Values &values = parser.parse_args(argc, argv);
        bool longHelp = values.get("long");
        longHelp = longHelp || (
            argv[1]         &&
            'm'==argv[1][0] &&
            'a'==argv[1][1] &&
            'n'==argv[1][2]
        );
        longHelp = longHelp || (
            argv[1]         &&
            'd'==argv[1][0] &&
            'o'==argv[1][1] &&
            'c'==argv[1][2]
        );

        auto args = parser.args();
        if(1<args.size()) {
            for(size_t i=1; i<args.size(); ++i) { // O(N^2), but short lists
                auto arg = args[i];
                Callback::showHelpFor(
                    arg.c_str(),
                    longHelp
                );
            }
        } else {
            printf("\n");
            printf("General Usage: parser <command> <options> <command arguments>\n");
            printf("\n");
            printf("    Where <command> can be any of:\n");
            Callback::find("", true);
            printf("\n");
            printf("    NOTE: use \"parser help <command>\" or \"parser <command> --help\" to get detailed\n");
            printf("          help for a specific command.\n");
            printf("\n");
            printf("    NOTE: <command> may have multiple aliases and can also be abbreviated. For\n");
            printf("          example, \"parser tx\", \"parser tr\", and \"parser transactions\" are equivalent.\n");
            printf("\n");
            printf("    NOTE: whenever specifying a list of things (e.g. a list of addresses), you can\n");
            printf("          instead enter \"file:list.txt\" and the list will be read from the file.\n");
            printf("\n");
            printf("    NOTE: whenever specifying a list file, you can use \"file:-\" and blockparser\n");
            printf("          will read the list directly from stdin.\n");
            printf("\n");
            printf("\n");

            if(longHelp) {
                printf("Every available command and its specific usage follows:\n");
                printf("\n");
                Callback::showAllHelps(false);
            }
        }
        return 0;
    }
};

static Help help;

