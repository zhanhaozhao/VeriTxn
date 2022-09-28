#pragma once

#include <map>
#include "global_enc.h"
#include "table.h"

using namespace std;

class table_map {
    // if concurrent problem will happen?
public:
    table_map();
    table_t* get_table(const char* name);
    void store_table(const char *name, table_t* tab);

private:
    map<string, void*>     _tables;
    map<string, void*>     _indexes;
};

