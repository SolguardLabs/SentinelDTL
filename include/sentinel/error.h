#ifndef SENTINEL_ERROR_H
#define SENTINEL_ERROR_H

#include "sentinel/types.h"

typedef enum sg_status {
    SG_OK = 0,
    SG_ERR_INVALID_ARGUMENT = 1,
    SG_ERR_NOT_FOUND = 2,
    SG_ERR_DUPLICATE = 3,
    SG_ERR_CAPACITY = 4,
    SG_ERR_OVERFLOW = 5,
    SG_ERR_INSUFFICIENT_BALANCE = 6,
    SG_ERR_LIMIT = 7,
    SG_ERR_PAUSED = 8,
    SG_ERR_PROTECTION = 9,
    SG_ERR_ORDER_STATE = 10,
    SG_ERR_VAULT_STATE = 11,
    SG_ERR_INTERNAL = 12
} sg_status_t;

typedef struct sg_error {
    sg_status_t status;
    char message[160];
} sg_error_t;

void sg_error_clear(sg_error_t *err);
void sg_error_set(sg_error_t *err, sg_status_t status, const char *message);
const char *sg_status_name(sg_status_t status);
bool sg_status_is_ok(sg_status_t status);

#endif
