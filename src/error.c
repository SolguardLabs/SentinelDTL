#include "sentinel/error.h"

#include <string.h>

void sg_error_clear(sg_error_t *err) {
    if (err == NULL) {
        return;
    }
    err->status = SG_OK;
    err->message[0] = '\0';
}

void sg_error_set(sg_error_t *err, sg_status_t status, const char *message) {
    if (err == NULL) {
        return;
    }
    err->status = status;
    if (message == NULL) {
        err->message[0] = '\0';
        return;
    }
    strncpy(err->message, message, sizeof(err->message) - 1);
    err->message[sizeof(err->message) - 1] = '\0';
}

const char *sg_status_name(sg_status_t status) {
    switch (status) {
        case SG_OK:
            return "ok";
        case SG_ERR_INVALID_ARGUMENT:
            return "invalid_argument";
        case SG_ERR_NOT_FOUND:
            return "not_found";
        case SG_ERR_DUPLICATE:
            return "duplicate";
        case SG_ERR_CAPACITY:
            return "capacity";
        case SG_ERR_OVERFLOW:
            return "overflow";
        case SG_ERR_INSUFFICIENT_BALANCE:
            return "insufficient_balance";
        case SG_ERR_LIMIT:
            return "limit";
        case SG_ERR_PAUSED:
            return "paused";
        case SG_ERR_PROTECTION:
            return "protection";
        case SG_ERR_ORDER_STATE:
            return "order_state";
        case SG_ERR_VAULT_STATE:
            return "vault_state";
        case SG_ERR_INTERNAL:
            return "internal";
    }
    return "unknown";
}

bool sg_status_is_ok(sg_status_t status) {
    return status == SG_OK;
}
