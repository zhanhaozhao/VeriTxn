#ifndef TABLE_MAP_H_
#define TABLE_MAP_H_

#include <map>
#include "table.h"

// using namespace std;

class table_map {
    // if concurrent problem will happen?
public:
    table_map() {
        _tables.clear();
    }

    table_t* get_table(const char* name) {
        auto re = _tables[std::string(name)];
        if (re == nullptr) {
        }
        return (table_t *) re;
    }

    void store_table(const char *name, table_t* tab) {
        _tables[std::string(name)] = (void*) tab;
    }

public:
    std::map<std::string, void*>     _tables;
    std::map<std::string, void*>     _indexes;
};

#endif