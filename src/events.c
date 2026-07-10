#include "sentinel/events.h"

#include <string.h>

static void sg_event_copy(char *dst, size_t cap, const char *src) {
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

void sg_event_log_init(sg_event_log_t *log) {
    if (log == NULL) {
        return;
    }
    memset(log, 0, sizeof(*log));
}

sg_status_t sg_event_push(
    sg_event_log_t *log,
    sg_event_kind_t kind,
    sg_time_t time,
    const char *subject,
    sg_amount_t amount,
    sg_amount_t aux_amount,
    sg_version_t version,
    const char *message,
    sg_error_t *err) {
    sg_event_t *event = NULL;
    if (log == NULL) {
        sg_error_set(err, SG_ERR_INVALID_ARGUMENT, "event log is required");
        return SG_ERR_INVALID_ARGUMENT;
    }
    if (log->len >= SG_MAX_EVENTS) {
        sg_error_set(err, SG_ERR_CAPACITY, "event log capacity reached");
        return SG_ERR_CAPACITY;
    }
    event = &log->items[log->len++];
    event->kind = kind;
    event->time = time;
    sg_id_from_text(&event->subject, subject);
    event->amount = amount;
    event->aux_amount = aux_amount;
    event->version = version;
    sg_event_copy(event->message, sizeof(event->message), message);
    return SG_OK;
}

const sg_event_t *sg_event_at(const sg_event_log_t *log, size_t index) {
    if (log == NULL || index >= log->len) {
        return NULL;
    }
    return &log->items[index];
}

size_t sg_event_count_kind(const sg_event_log_t *log, sg_event_kind_t kind) {
    size_t count = 0;
    size_t index = 0;
    if (log == NULL) {
        return 0;
    }
    for (index = 0; index < log->len; index++) {
        if (log->items[index].kind == kind) {
            count += 1;
        }
    }
    return count;
}
