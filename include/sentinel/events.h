#ifndef SENTINEL_EVENTS_H
#define SENTINEL_EVENTS_H

#include "sentinel/order.h"

typedef struct sg_event {
    sg_event_kind_t kind;
    sg_time_t time;
    sg_id_t subject;
    sg_amount_t amount;
    sg_amount_t aux_amount;
    sg_version_t version;
    char message[SG_LABEL_LEN];
} sg_event_t;

typedef struct sg_event_log {
    sg_event_t items[SG_MAX_EVENTS];
    size_t len;
} sg_event_log_t;

void sg_event_log_init(sg_event_log_t *log);
sg_status_t sg_event_push(
    sg_event_log_t *log,
    sg_event_kind_t kind,
    sg_time_t time,
    const char *subject,
    sg_amount_t amount,
    sg_amount_t aux_amount,
    sg_version_t version,
    const char *message,
    sg_error_t *err);
const sg_event_t *sg_event_at(const sg_event_log_t *log, size_t index);
size_t sg_event_count_kind(const sg_event_log_t *log, sg_event_kind_t kind);

#endif
