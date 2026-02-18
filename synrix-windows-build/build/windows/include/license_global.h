#ifndef SYNRIX_LICENSE_GLOBAL_H
#define SYNRIX_LICENSE_GLOBAL_H

#include <stdint.h>

int license_global_register(uint32_t node_count, uint32_t limit);
int license_global_add_one(uint32_t limit);

#endif
