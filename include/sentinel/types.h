#ifndef SENTINEL_TYPES_H
#define SENTINEL_TYPES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define SG_ID_LEN 40
#define SG_LABEL_LEN 64
#define SG_MAX_ACCOUNTS 16
#define SG_MAX_SESSIONS 16
#define SG_MAX_ORDERS 96
#define SG_MAX_EVENTS 192
#define SG_MAX_JSON_DEPTH 32

typedef uint64_t sg_amount_t;
typedef uint64_t sg_time_t;
typedef uint64_t sg_version_t;

typedef enum sg_control_mode {
    SG_CONTROL_ACTIVE = 0,
    SG_CONTROL_PAUSED = 1,
    SG_CONTROL_PROTECTION = 2
} sg_control_mode_t;

typedef enum sg_session_state {
    SG_SESSION_OPEN = 0,
    SG_SESSION_PAUSED = 1,
    SG_SESSION_PROTECTION = 2,
    SG_SESSION_CLOSED = 3
} sg_session_state_t;

typedef enum sg_order_kind {
    SG_ORDER_REDEEM_RISK = 0,
    SG_ORDER_LOCK_BUFFER = 1
} sg_order_kind_t;

typedef enum sg_order_status {
    SG_ORDER_EMPTY = 0,
    SG_ORDER_QUEUED = 1,
    SG_ORDER_EXECUTED = 2,
    SG_ORDER_CANCELLED = 3,
    SG_ORDER_REJECTED = 4
} sg_order_status_t;

typedef enum sg_event_kind {
    SG_EVENT_BOOT = 0,
    SG_EVENT_ACCOUNT_CREATED = 1,
    SG_EVENT_SESSION_OPENED = 2,
    SG_EVENT_ORDER_QUEUED = 3,
    SG_EVENT_ORDER_EXECUTED = 4,
    SG_EVENT_ORDER_REJECTED = 5,
    SG_EVENT_PAUSED = 6,
    SG_EVENT_RESUMED = 7,
    SG_EVENT_PROTECTION_ENTERED = 8,
    SG_EVENT_VAULT_ADJUSTED = 9,
    SG_EVENT_ORDER_CANCELLED = 10,
    SG_EVENT_CHECKPOINT = 11
} sg_event_kind_t;

typedef struct sg_id {
    char value[SG_ID_LEN];
} sg_id_t;

void sg_id_from_text(sg_id_t *id, const char *text);
bool sg_id_eq(const sg_id_t *left, const sg_id_t *right);
bool sg_id_is_empty(const sg_id_t *id);
const char *sg_control_mode_name(sg_control_mode_t mode);
const char *sg_session_state_name(sg_session_state_t state);
const char *sg_order_kind_name(sg_order_kind_t kind);
const char *sg_order_status_name(sg_order_status_t status);
const char *sg_event_kind_name(sg_event_kind_t kind);

#endif
