#ifndef SENTINEL_SCENARIO_H
#define SENTINEL_SCENARIO_H

#include "sentinel/json.h"

sg_status_t sg_scenario_run(const char *name, FILE *out, sg_error_t *err);
void sg_scenario_print_list(FILE *out);

#endif
