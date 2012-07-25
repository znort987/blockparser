
// Dump help

#include <stdio.h>
#include <common.h>
#include <option.h>
#include <callback.h>

#define CBNAME "help"
enum  optionIndex { kUnknown };
static const option::Descriptor usageDescriptor[] =
{
    { kUnknown, 0, "", "", option::Arg::None, "\n\n        print this help message" },
    { 0,        0,  0,  0,                 0,                                            0 }
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
        if(parse.error()) exit(1);

        printf("\n");
        printf("Usage: parser <command> <command options> <command data>\n");
        printf("\n");
        printf("    Note that <command> can be abbreviated and has mutlipled aliases.\n");
        printf("    For example, \"parser tx\", \"parser tr\", and \"parser transactions\" are equivalent.\n");
        printf("\n");
        printf("Available commands:\n");
        printf("\n");

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

