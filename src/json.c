#include "sentinel/json.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static sg_status_t sg_json_reserve(sg_json_t *json, size_t extra, sg_error_t *err) {
    size_t next_cap = 0;
    char *next = NULL;
    if (json == NULL) {
        sg_error_set(err, SG_ERR_INVALID_ARGUMENT, "json builder is required");
        return SG_ERR_INVALID_ARGUMENT;
    }
    if (json->len + extra + 1 <= json->cap) {
        return SG_OK;
    }
    next_cap = json->cap == 0 ? 1024 : json->cap;
    while (json->len + extra + 1 > next_cap) {
        if (next_cap > SIZE_MAX / 2) {
            sg_error_set(err, SG_ERR_OVERFLOW, "json buffer capacity overflow");
            return SG_ERR_OVERFLOW;
        }
        next_cap *= 2;
    }
    next = (char *)realloc(json->data, next_cap);
    if (next == NULL) {
        sg_error_set(err, SG_ERR_CAPACITY, "json buffer allocation failed");
        return SG_ERR_CAPACITY;
    }
    json->data = next;
    json->cap = next_cap;
    return SG_OK;
}

static sg_status_t sg_json_append_raw(sg_json_t *json, const char *text, sg_error_t *err) {
    size_t len = 0;
    sg_status_t status = SG_OK;
    if (text == NULL) {
        text = "";
    }
    len = strlen(text);
    status = sg_json_reserve(json, len, err);
    if (status != SG_OK) {
        return status;
    }
    memcpy(json->data + json->len, text, len);
    json->len += len;
    json->data[json->len] = '\0';
    return SG_OK;
}

static sg_status_t sg_json_append_char(sg_json_t *json, char ch, sg_error_t *err) {
    sg_status_t status = sg_json_reserve(json, 1, err);
    if (status != SG_OK) {
        return status;
    }
    json->data[json->len++] = ch;
    json->data[json->len] = '\0';
    return SG_OK;
}

static sg_status_t sg_json_append_fmt(sg_json_t *json, sg_error_t *err, const char *fmt, ...) {
    va_list args;
    va_list copy;
    int needed = 0;
    sg_status_t status = SG_OK;
    va_start(args, fmt);
    va_copy(copy, args);
    needed = vsnprintf(NULL, 0, fmt, copy);
    va_end(copy);
    if (needed < 0) {
        va_end(args);
        sg_error_set(err, SG_ERR_INTERNAL, "json formatting failed");
        return SG_ERR_INTERNAL;
    }
    status = sg_json_reserve(json, (size_t)needed, err);
    if (status != SG_OK) {
        va_end(args);
        return status;
    }
    (void)vsnprintf(json->data + json->len, json->cap - json->len, fmt, args);
    va_end(args);
    json->len += (size_t)needed;
    return SG_OK;
}

static sg_status_t sg_json_before_value(sg_json_t *json, sg_error_t *err) {
    if (json->depth == 0) {
        return SG_OK;
    }
    if (json->needs_comma[json->depth - 1]) {
        sg_status_t status = sg_json_append_char(json, ',', err);
        if (status != SG_OK) {
            return status;
        }
    }
    json->needs_comma[json->depth - 1] = true;
    return SG_OK;
}

static sg_status_t sg_json_write_escaped(sg_json_t *json, const char *value, sg_error_t *err) {
    const unsigned char *cursor = (const unsigned char *)(value == NULL ? "" : value);
    sg_status_t status = sg_json_append_char(json, '"', err);
    if (status != SG_OK) {
        return status;
    }
    while (*cursor != '\0') {
        unsigned char ch = *cursor++;
        switch (ch) {
            case '"':
                status = sg_json_append_raw(json, "\\\"", err);
                break;
            case '\\':
                status = sg_json_append_raw(json, "\\\\", err);
                break;
            case '\b':
                status = sg_json_append_raw(json, "\\b", err);
                break;
            case '\f':
                status = sg_json_append_raw(json, "\\f", err);
                break;
            case '\n':
                status = sg_json_append_raw(json, "\\n", err);
                break;
            case '\r':
                status = sg_json_append_raw(json, "\\r", err);
                break;
            case '\t':
                status = sg_json_append_raw(json, "\\t", err);
                break;
            default:
                if (ch < 0x20) {
                    status = sg_json_append_fmt(json, err, "\\u%04x", ch);
                } else {
                    status = sg_json_append_char(json, (char)ch, err);
                }
                break;
        }
        if (status != SG_OK) {
            return status;
        }
    }
    return sg_json_append_char(json, '"', err);
}

static sg_status_t sg_json_member_prefix(sg_json_t *json, const char *key, sg_error_t *err) {
    sg_status_t status = SG_OK;
    if (json == NULL || key == NULL || json->depth == 0) {
        sg_error_set(err, SG_ERR_INVALID_ARGUMENT, "json member key requires an object");
        return SG_ERR_INVALID_ARGUMENT;
    }
    status = sg_json_before_value(json, err);
    if (status != SG_OK) {
        return status;
    }
    status = sg_json_write_escaped(json, key, err);
    if (status != SG_OK) {
        return status;
    }
    return sg_json_append_char(json, ':', err);
}

static sg_status_t sg_json_begin_object_key(sg_json_t *json, const char *key, sg_error_t *err) {
    sg_status_t status = sg_json_member_prefix(json, key, err);
    if (status != SG_OK) {
        return status;
    }
    if (json->depth >= SG_MAX_JSON_DEPTH) {
        sg_error_set(err, SG_ERR_CAPACITY, "json depth reached");
        return SG_ERR_CAPACITY;
    }
    status = sg_json_append_char(json, '{', err);
    if (status != SG_OK) {
        return status;
    }
    json->needs_comma[json->depth++] = false;
    return SG_OK;
}

void sg_json_init(sg_json_t *json) {
    if (json == NULL) {
        return;
    }
    memset(json, 0, sizeof(*json));
}

void sg_json_free(sg_json_t *json) {
    if (json == NULL) {
        return;
    }
    free(json->data);
    memset(json, 0, sizeof(*json));
}

sg_status_t sg_json_begin_object(sg_json_t *json, sg_error_t *err) {
    sg_status_t status = SG_OK;
    if (json == NULL) {
        sg_error_set(err, SG_ERR_INVALID_ARGUMENT, "json builder is required");
        return SG_ERR_INVALID_ARGUMENT;
    }
    if (json->depth >= SG_MAX_JSON_DEPTH) {
        sg_error_set(err, SG_ERR_CAPACITY, "json depth reached");
        return SG_ERR_CAPACITY;
    }
    status = sg_json_before_value(json, err);
    if (status != SG_OK) {
        return status;
    }
    status = sg_json_append_char(json, '{', err);
    if (status != SG_OK) {
        return status;
    }
    json->needs_comma[json->depth++] = false;
    return SG_OK;
}

sg_status_t sg_json_end_object(sg_json_t *json, sg_error_t *err) {
    if (json == NULL || json->depth == 0) {
        sg_error_set(err, SG_ERR_INVALID_ARGUMENT, "json object close is invalid");
        return SG_ERR_INVALID_ARGUMENT;
    }
    json->depth -= 1;
    return sg_json_append_char(json, '}', err);
}

sg_status_t sg_json_begin_array(sg_json_t *json, const char *key, sg_error_t *err) {
    sg_status_t status = SG_OK;
    if (key != NULL) {
        status = sg_json_member_prefix(json, key, err);
        if (status != SG_OK) {
            return status;
        }
    } else {
        status = sg_json_before_value(json, err);
        if (status != SG_OK) {
            return status;
        }
    }
    if (json->depth >= SG_MAX_JSON_DEPTH) {
        sg_error_set(err, SG_ERR_CAPACITY, "json depth reached");
        return SG_ERR_CAPACITY;
    }
    status = sg_json_append_char(json, '[', err);
    if (status != SG_OK) {
        return status;
    }
    json->needs_comma[json->depth++] = false;
    return SG_OK;
}

sg_status_t sg_json_end_array(sg_json_t *json, sg_error_t *err) {
    if (json == NULL || json->depth == 0) {
        sg_error_set(err, SG_ERR_INVALID_ARGUMENT, "json array close is invalid");
        return SG_ERR_INVALID_ARGUMENT;
    }
    json->depth -= 1;
    return sg_json_append_char(json, ']', err);
}

sg_status_t sg_json_key_string(sg_json_t *json, const char *key, const char *value, sg_error_t *err) {
    sg_status_t status = sg_json_member_prefix(json, key, err);
    if (status != SG_OK) {
        return status;
    }
    return sg_json_write_escaped(json, value, err);
}

sg_status_t sg_json_key_u64(sg_json_t *json, const char *key, uint64_t value, sg_error_t *err) {
    sg_status_t status = sg_json_member_prefix(json, key, err);
    if (status != SG_OK) {
        return status;
    }
    return sg_json_append_fmt(json, err, "%llu", (unsigned long long)value);
}

sg_status_t sg_json_key_bool(sg_json_t *json, const char *key, bool value, sg_error_t *err) {
    sg_status_t status = sg_json_member_prefix(json, key, err);
    if (status != SG_OK) {
        return status;
    }
    return sg_json_append_raw(json, value ? "true" : "false", err);
}

sg_status_t sg_json_array_string(sg_json_t *json, const char *value, sg_error_t *err) {
    sg_status_t status = sg_json_before_value(json, err);
    if (status != SG_OK) {
        return status;
    }
    return sg_json_write_escaped(json, value, err);
}

sg_status_t sg_json_array_u64(sg_json_t *json, uint64_t value, sg_error_t *err) {
    sg_status_t status = sg_json_before_value(json, err);
    if (status != SG_OK) {
        return status;
    }
    return sg_json_append_fmt(json, err, "%llu", (unsigned long long)value);
}

static sg_status_t sg_json_write_policy(
    sg_json_t *json,
    const sg_policy_t *policy,
    sg_error_t *err) {
    sg_status_t status = sg_json_begin_object_key(json, "policy", err);
    if (status != SG_OK) return status;
    if ((status = sg_json_key_u64(json, "session_limit", policy->session_limit, err)) != SG_OK) return status;
    if ((status = sg_json_key_u64(json, "single_order_limit", policy->single_order_limit, err)) != SG_OK) return status;
    if ((status = sg_json_key_u64(json, "queue_limit", policy->queue_limit, err)) != SG_OK) return status;
    if ((status = sg_json_key_u64(json, "reserve_floor", policy->reserve_floor, err)) != SG_OK) return status;
    if ((status = sg_json_key_u64(json, "execution_fee_bps", policy->execution_fee_bps, err)) != SG_OK) return status;
    if ((status = sg_json_key_bool(json, "execute_during_protection", policy->execute_during_protection, err)) != SG_OK) return status;
    return sg_json_end_object(json, err);
}

static sg_status_t sg_json_write_vault(
    sg_json_t *json,
    const sg_vault_t *vault,
    sg_error_t *err) {
    sg_amount_t unit_value = 0;
    sg_status_t status = SG_OK;
    sg_error_t local;
    sg_error_clear(&local);
    (void)sg_vault_unit_value(vault, &unit_value, &local);
    status = sg_json_begin_object_key(json, "vault", err);
    if (status != SG_OK) return status;
    if ((status = sg_json_key_string(json, "id", vault->id.value, err)) != SG_OK) return status;
    if ((status = sg_json_key_u64(json, "reserve_cash", vault->reserve_cash, err)) != SG_OK) return status;
    if ((status = sg_json_key_u64(json, "issued_risk_units", vault->issued_risk_units, err)) != SG_OK) return status;
    if ((status = sg_json_key_u64(json, "blocked_cash", vault->blocked_cash, err)) != SG_OK) return status;
    if ((status = sg_json_key_u64(json, "reserve_floor", vault->reserve_floor, err)) != SG_OK) return status;
    if ((status = sg_json_key_u64(json, "unit_value", unit_value, err)) != SG_OK) return status;
    if ((status = sg_json_key_u64(json, "haircut_bps", vault->haircut_bps, err)) != SG_OK) return status;
    if ((status = sg_json_key_u64(json, "version", vault->version, err)) != SG_OK) return status;
    if ((status = sg_json_key_bool(json, "floor_ok", sg_vault_reserve_floor_ok(vault), err)) != SG_OK) return status;
    return sg_json_end_object(json, err);
}

static sg_status_t sg_json_write_risk(
    sg_json_t *json,
    const sg_engine_t *engine,
    sg_error_t *err) {
    sg_risk_snapshot_t snapshot;
    sg_status_t status = SG_OK;
    status = sg_engine_risk_snapshot(engine, &snapshot, err);
    if (status != SG_OK) return status;
    status = sg_json_begin_object_key(json, "risk", err);
    if (status != SG_OK) return status;
    if ((status = sg_json_key_u64(json, "total_cash", snapshot.total_cash, err)) != SG_OK) return status;
    if ((status = sg_json_key_u64(json, "account_cash", snapshot.account_cash, err)) != SG_OK) return status;
    if ((status = sg_json_key_u64(json, "vault_cash", snapshot.vault_cash, err)) != SG_OK) return status;
    if ((status = sg_json_key_u64(json, "vault_liquid_cash", snapshot.vault_liquid_cash, err)) != SG_OK) return status;
    if ((status = sg_json_key_u64(json, "reserve_headroom", snapshot.reserve_headroom, err)) != SG_OK) return status;
    if ((status = sg_json_key_u64(json, "total_risk_units", snapshot.total_risk_units, err)) != SG_OK) return status;
    if ((status = sg_json_key_u64(json, "queued_risk_units", snapshot.queued_risk_units, err)) != SG_OK) return status;
    if ((status = sg_json_key_u64(json, "live_order_notional", snapshot.live_order_notional, err)) != SG_OK) return status;
    if ((status = sg_json_key_u64(json, "live_order_units", snapshot.live_order_units, err)) != SG_OK) return status;
    if ((status = sg_json_key_u64(json, "session_utilization_bps", snapshot.session_utilization_bps, err)) != SG_OK) return status;
    if ((status = sg_json_key_u64(json, "queue_utilization_bps", snapshot.queue_utilization_bps, err)) != SG_OK) return status;
    if ((status = sg_json_key_u64(json, "reserve_floor_bps", snapshot.reserve_floor_bps, err)) != SG_OK) return status;
    if ((status = sg_json_key_string(json, "session_risk_band", sg_risk_band_name(snapshot.session_utilization_bps), err)) != SG_OK) return status;
    if ((status = sg_json_key_u64(json, "open_sessions", snapshot.open_sessions, err)) != SG_OK) return status;
    if ((status = sg_json_key_u64(json, "protected_sessions", snapshot.protected_sessions, err)) != SG_OK) return status;
    if ((status = sg_json_key_u64(json, "live_orders", snapshot.live_orders, err)) != SG_OK) return status;
    if ((status = sg_json_key_bool(json, "balanced", sg_risk_snapshot_is_balanced(&snapshot), err)) != SG_OK) return status;
    return sg_json_end_object(json, err);
}

static sg_status_t sg_json_write_account(
    sg_json_t *json,
    const sg_account_t *account,
    sg_error_t *err) {
    sg_status_t status = sg_json_begin_object(json, err);
    if (status != SG_OK) return status;
    if ((status = sg_json_key_string(json, "id", account->id.value, err)) != SG_OK) return status;
    if ((status = sg_json_key_u64(json, "cash_balance", account->cash_balance, err)) != SG_OK) return status;
    if ((status = sg_json_key_u64(json, "risk_units", account->risk_units, err)) != SG_OK) return status;
    if ((status = sg_json_key_u64(json, "queued_units", account->queued_units, err)) != SG_OK) return status;
    if ((status = sg_json_key_u64(json, "credited_cash", account->credited_cash, err)) != SG_OK) return status;
    if ((status = sg_json_key_u64(json, "debited_cash", account->debited_cash, err)) != SG_OK) return status;
    return sg_json_end_object(json, err);
}

static sg_status_t sg_json_write_session(
    sg_json_t *json,
    const sg_session_t *session,
    const sg_policy_t *policy,
    sg_error_t *err) {
    sg_status_t status = sg_json_begin_object(json, err);
    if (status != SG_OK) return status;
    if ((status = sg_json_key_string(json, "id", session->id.value, err)) != SG_OK) return status;
    if ((status = sg_json_key_string(json, "owner", session->owner.value, err)) != SG_OK) return status;
    if ((status = sg_json_key_string(json, "state", sg_session_state_name(session->state), err)) != SG_OK) return status;
    if ((status = sg_json_key_u64(json, "settled_notional", session->settled_notional, err)) != SG_OK) return status;
    if ((status = sg_json_key_u64(json, "queued_notional", session->queued_notional, err)) != SG_OK) return status;
    if ((status = sg_json_key_u64(json, "rejected_notional", session->rejected_notional, err)) != SG_OK) return status;
    if ((status = sg_json_key_u64(json, "frozen_settled_notional", session->frozen_settled_notional, err)) != SG_OK) return status;
    if ((status = sg_json_key_u64(json, "frozen_queued_notional", session->frozen_queued_notional, err)) != SG_OK) return status;
    if ((status = sg_json_key_u64(json, "available_limit", sg_session_available_limit(session, policy), err)) != SG_OK) return status;
    if ((status = sg_json_key_u64(json, "queued_orders", session->queued_orders, err)) != SG_OK) return status;
    if ((status = sg_json_key_u64(json, "executed_orders", session->executed_orders, err)) != SG_OK) return status;
    if ((status = sg_json_key_u64(json, "cancelled_orders", session->cancelled_orders, err)) != SG_OK) return status;
    if ((status = sg_json_key_u64(json, "rejected_orders", session->rejected_orders, err)) != SG_OK) return status;
    if ((status = sg_json_key_u64(json, "pause_count", session->pause_count, err)) != SG_OK) return status;
    if ((status = sg_json_key_u64(json, "resume_count", session->resume_count, err)) != SG_OK) return status;
    if ((status = sg_json_key_u64(json, "protection_count", session->protection_count, err)) != SG_OK) return status;
    return sg_json_end_object(json, err);
}

static sg_status_t sg_json_write_order(
    sg_json_t *json,
    const sg_order_t *order,
    sg_error_t *err) {
    sg_status_t status = sg_json_begin_object(json, err);
    if (status != SG_OK) return status;
    if ((status = sg_json_key_string(json, "id", order->id.value, err)) != SG_OK) return status;
    if ((status = sg_json_key_string(json, "session_id", order->session_id.value, err)) != SG_OK) return status;
    if ((status = sg_json_key_string(json, "owner", order->owner.value, err)) != SG_OK) return status;
    if ((status = sg_json_key_string(json, "kind", sg_order_kind_name(order->kind), err)) != SG_OK) return status;
    if ((status = sg_json_key_string(json, "status", sg_order_status_name(order->status), err)) != SG_OK) return status;
    if ((status = sg_json_key_u64(json, "risk_units", order->risk_units, err)) != SG_OK) return status;
    if ((status = sg_json_key_u64(json, "admission_notional", order->admission_notional, err)) != SG_OK) return status;
    if ((status = sg_json_key_u64(json, "execution_notional", order->execution_notional, err)) != SG_OK) return status;
    if ((status = sg_json_key_u64(json, "min_cash_out", order->min_cash_out, err)) != SG_OK) return status;
    if ((status = sg_json_key_u64(json, "fee_charged", order->fee_charged, err)) != SG_OK) return status;
    if ((status = sg_json_key_u64(json, "admitted_vault_version", order->admitted_vault_version, err)) != SG_OK) return status;
    if ((status = sg_json_key_u64(json, "executed_vault_version", order->executed_vault_version, err)) != SG_OK) return status;
    if ((status = sg_json_key_u64(json, "queued_at", order->queued_at, err)) != SG_OK) return status;
    if ((status = sg_json_key_u64(json, "executed_at", order->executed_at, err)) != SG_OK) return status;
    if ((status = sg_json_key_string(json, "note", order->note, err)) != SG_OK) return status;
    return sg_json_end_object(json, err);
}

static sg_status_t sg_json_write_event(
    sg_json_t *json,
    const sg_event_t *event,
    sg_error_t *err) {
    sg_status_t status = sg_json_begin_object(json, err);
    if (status != SG_OK) return status;
    if ((status = sg_json_key_string(json, "kind", sg_event_kind_name(event->kind), err)) != SG_OK) return status;
    if ((status = sg_json_key_u64(json, "time", event->time, err)) != SG_OK) return status;
    if ((status = sg_json_key_string(json, "subject", event->subject.value, err)) != SG_OK) return status;
    if ((status = sg_json_key_u64(json, "amount", event->amount, err)) != SG_OK) return status;
    if ((status = sg_json_key_u64(json, "aux_amount", event->aux_amount, err)) != SG_OK) return status;
    if ((status = sg_json_key_u64(json, "version", event->version, err)) != SG_OK) return status;
    if ((status = sg_json_key_string(json, "message", event->message, err)) != SG_OK) return status;
    return sg_json_end_object(json, err);
}

sg_status_t sg_json_write_engine(
    sg_json_t *json,
    const sg_engine_t *engine,
    const char *scenario,
    const sg_error_t *last_error,
    sg_error_t *err) {
    size_t index = 0;
    sg_status_t status = SG_OK;
    if (json == NULL || engine == NULL) {
        sg_error_set(err, SG_ERR_INVALID_ARGUMENT, "json builder and engine are required");
        return SG_ERR_INVALID_ARGUMENT;
    }
    if ((status = sg_json_begin_object(json, err)) != SG_OK) return status;
    if ((status = sg_json_key_string(json, "scenario", scenario, err)) != SG_OK) return status;
    if ((status = sg_json_key_string(json, "mode", sg_control_mode_name(engine->mode), err)) != SG_OK) return status;
    if ((status = sg_json_key_u64(json, "clock", engine->clock, err)) != SG_OK) return status;
    if ((status = sg_json_write_policy(json, &engine->policy, err)) != SG_OK) return status;
    if ((status = sg_json_write_vault(json, &engine->vault, err)) != SG_OK) return status;
    if ((status = sg_json_write_risk(json, engine, err)) != SG_OK) return status;

    if ((status = sg_json_begin_array(json, "accounts", err)) != SG_OK) return status;
    for (index = 0; index < engine->account_len; index++) {
        if ((status = sg_json_write_account(json, &engine->accounts[index], err)) != SG_OK) return status;
    }
    if ((status = sg_json_end_array(json, err)) != SG_OK) return status;

    if ((status = sg_json_begin_array(json, "sessions", err)) != SG_OK) return status;
    for (index = 0; index < engine->session_len; index++) {
        if ((status = sg_json_write_session(json, &engine->sessions[index], &engine->policy, err)) != SG_OK) return status;
    }
    if ((status = sg_json_end_array(json, err)) != SG_OK) return status;

    if ((status = sg_json_begin_array(json, "orders", err)) != SG_OK) return status;
    for (index = 0; index < engine->order_len; index++) {
        if ((status = sg_json_write_order(json, &engine->orders[index], err)) != SG_OK) return status;
    }
    if ((status = sg_json_end_array(json, err)) != SG_OK) return status;

    if ((status = sg_json_begin_array(json, "events", err)) != SG_OK) return status;
    for (index = 0; index < engine->events.len; index++) {
        if ((status = sg_json_write_event(json, &engine->events.items[index], err)) != SG_OK) return status;
    }
    if ((status = sg_json_end_array(json, err)) != SG_OK) return status;

    if ((status = sg_json_begin_object_key(json, "checks", err)) != SG_OK) return status;
    if ((status = sg_json_key_bool(json, "cash_conservation", sg_engine_cash_conservation_ok(engine), err)) != SG_OK) return status;
    if ((status = sg_json_key_bool(json, "unit_conservation", sg_engine_unit_conservation_ok(engine), err)) != SG_OK) return status;
    if ((status = sg_json_key_u64(json, "account_cash_total", sg_engine_total_account_cash(engine), err)) != SG_OK) return status;
    if ((status = sg_json_key_u64(json, "account_unit_total", sg_engine_total_account_units(engine), err)) != SG_OK) return status;
    if ((status = sg_json_key_u64(json, "expected_cash_supply", engine->expected_cash_supply, err)) != SG_OK) return status;
    if ((status = sg_json_end_object(json, err)) != SG_OK) return status;

    if ((status = sg_json_begin_object_key(json, "last_error", err)) != SG_OK) return status;
    if (last_error == NULL) {
        if ((status = sg_json_key_string(json, "status", "ok", err)) != SG_OK) return status;
        if ((status = sg_json_key_string(json, "message", "", err)) != SG_OK) return status;
    } else {
        if ((status = sg_json_key_string(json, "status", sg_status_name(last_error->status), err)) != SG_OK) return status;
        if ((status = sg_json_key_string(json, "message", last_error->message, err)) != SG_OK) return status;
    }
    if ((status = sg_json_end_object(json, err)) != SG_OK) return status;
    return sg_json_end_object(json, err);
}
