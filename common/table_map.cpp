#include "table_map.h"
#include "string"

table_map::table_map() {
    _tables.clear();
}

table_t *table_map::get_table(const char *name) {
    auto re = _tables[string(name)];
    if (re == nullptr) {
    }
    return (table_t *) re;
}

void table_map::store_table(const char *name, table_t *tab) {
    _tables[string(name)] = (void*)tab;
}
