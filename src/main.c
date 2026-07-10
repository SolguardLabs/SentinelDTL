#include "sentinel/sentinel.h"

#include <stdio.h>
#include <string.h>

int main(int argc, char **argv) {
    const char *scenario = "baseline";
    sg_error_t err;
    sg_status_t status = SG_OK;
    sg_error_clear(&err);
    if (argc > 1) {
        scenario = argv[1];
    }
    if (strcmp(scenario, "--list") == 0 || strcmp(scenario, "list") == 0) {
        sg_scenario_print_list(stdout);
        return 0;
    }
    status = sg_scenario_run(scenario, stdout, &err);
    if (status != SG_OK) {
        fprintf(
            stderr,
            "{\"status\":\"%s\",\"message\":\"%s\"}\n",
            sg_status_name(err.status),
            err.message);
        return 1;
    }
    return 0;
}
