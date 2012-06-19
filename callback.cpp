
#include <map>
#include <string>
#include <callback.h>

typedef std::map<std::string, Callback*> Map;
static Map *map;

void Callback::add(
    const char *name,
    Callback *callback
)
{
    if(0==map)
        map = new Map;

    (*map)[std::string(name)] = callback;
}

Callback *Callback::find(
    const char *name
)
{
    if(!map)
        return 0;

    auto i = map->find(std::string(name));
    return (map->end()==i) ? 0 : i->second;
}

