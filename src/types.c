#include "sentinel/types.h"

#include <string.h>

static void sg_copy_text(char *dst, size_t cap, const char *src) {
    if (cap == 0) {
        return;
    }
    if (src == NULL) {
        dst[0] = '\0';
        return;
    }
    strncpy(dst, src, cap - 1);
    dst[cap - 1] = '\0';
}

void sg_id_from_text(sg_id_t *id, const char *text) {
    if (id == NULL) {
        return;
    }
    sg_copy_text(id->value, sizeof(id->value), text);
}

bool sg_id_eq(const sg_id_t *left, const sg_id_t *right) {
    if (left == NULL || right == NULL) {
        return false;
    }
    return strncmp(left->value, right->value, SG_ID_LEN) == 0;
}

bool sg_id_is_empty(const sg_id_t *id) {
    return id == NULL || id->value[0] == '\0';
}

const char *sg_control_mode_name(sg_control_mode_t mode) {
    switch (mode) {
        case SG_CONTROL_ACTIVE:
            return "active";
        case SG_CONTROL_PAUSED:
            return "paused";
        case SG_CONTROL_PROTECTION:
            return "protection";
    }
    return "unknown";
}

const char *sg_session_state_name(sg_session_state_t state) {
    switch (state) {
        case SG_SESSION_OPEN:
            return "open";
        case SG_SESSION_PAUSED:
            return "paused";
        case SG_SESSION_PROTECTION:
            return "protection";
        case SG_SESSION_CLOSED:
            return "closed";
    }
    return "unknown";
}

const char *sg_order_kind_name(sg_order_kind_t kind) {
    switch (kind) {
        case SG_ORDER_REDEEM_RISK:
            return "redeem_risk";
        case SG_ORDER_LOCK_BUFFER:
            return "lock_buffer";
    }
    return "unknown";
}

const char *sg_order_status_name(sg_order_status_t status) {
    switch (status) {
        case SG_ORDER_EMPTY:
            return "empty";
        case SG_ORDER_QUEUED:
            return "queued";
        case SG_ORDER_EXECUTED:
            return "executed";
        case SG_ORDER_CANCELLED:
            return "cancelled";
        case SG_ORDER_REJECTED:
            return "rejected";
    }
    return "unknown";
}

const char *sg_event_kind_name(sg_event_kind_t kind) {
    switch (kind) {
        case SG_EVENT_BOOT:
            return "boot";
        case SG_EVENT_ACCOUNT_CREATED:
            return "account_created";
        case SG_EVENT_SESSION_OPENED:
            return "session_opened";
        case SG_EVENT_ORDER_QUEUED:
            return "order_queued";
        case SG_EVENT_ORDER_EXECUTED:
            return "order_executed";
        case SG_EVENT_ORDER_REJECTED:
            return "order_rejected";
        case SG_EVENT_PAUSED:
            return "paused";
        case SG_EVENT_RESUMED:
            return "resumed";
        case SG_EVENT_PROTECTION_ENTERED:
            return "protection_entered";
        case SG_EVENT_VAULT_ADJUSTED:
            return "vault_adjusted";
        case SG_EVENT_ORDER_CANCELLED:
            return "order_cancelled";
        case SG_EVENT_CHECKPOINT:
            return "checkpoint";
    }
    return "unknown";
}
