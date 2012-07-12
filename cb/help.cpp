
// Dump help

#include <common.h>
#include <option.h>
#include <callback.h>

#define CBNAME "help"
enum  optionIndex { kUnknown };
static const option::Descriptor usageDescriptor[] =
{
    { kUnknown, 0, "", "", option::Arg::None, CBNAME ":\n" },
    { 0,        0,  0,  0,                 0,        0     }
};

struct Help:public Callback
{
    virtual bool needTXHash()
    {
        return false;
    }

    virtual int init(
        int  argc,
        char *argv[]
    )
    {
        option::Stats  stats(usageDescriptor, argc, argv);
        option::Option *buffer  = new option::Option[stats.buffer_max];
        option::Option *options = new option::Option[stats.options_max];
        option::Parser parse(usageDescriptor, argc, argv, options, buffer);

        if(parse.error())
            exit(1);

        Callback::showAllHelps();
        delete [] options;
        delete [] buffer;
        exit(0);
    }

    virtual const option::Descriptor *usage() const
    {
        return usageDescriptor;
    }

    virtual const char *name() const
    {
        return CBNAME;
    }
};

static Help help;

