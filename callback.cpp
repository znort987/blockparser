
#include <vector>
#include <string.h>
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

