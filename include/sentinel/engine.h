#ifndef SENTINEL_ENGINE_H
#define SENTINEL_ENGINE_H

#include "sentinel/events.h"
#include "sentinel/risk.h"
#include "sentinel/session.h"

typedef struct sg_account {
    sg_id_t id;
    sg_amount_t cash_balance;
    sg_amount_t risk_units;
    sg_amount_t queued_units;
    sg_amount_t credited_cash;
    sg_amount_t debited_cash;
} sg_account_t;

typedef struct sg_engine {
    sg_policy_t policy;
    sg_vault_t vault;
    sg_account_t accounts[SG_MAX_ACCOUNTS];
    sg_session_t sessions[SG_MAX_SESSIONS];
    sg_order_t orders[SG_MAX_ORDERS];
    sg_event_log_t events;
    sg_control_mode_t mode;
    sg_time_t clock;
    sg_amount_t expected_cash_supply;
    size_t account_len;
    size_t session_len;
    size_t order_len;
    uint64_t next_order_seq;
} sg_engine_t;

void sg_engine_init(sg_engine_t *engine);
sg_status_t sg_engine_bootstrap(sg_engine_t *engine, sg_error_t *err);
sg_status_t sg_engine_add_account(
    sg_engine_t *engine,
    const char *id,
    sg_amount_t cash_balance,
    sg_amount_t risk_units,
    sg_error_t *err);
sg_account_t *sg_engine_find_account(sg_engine_t *engine, const char *id);
const sg_account_t *sg_engine_find_account_const(const sg_engine_t *engine, const char *id);
sg_session_t *sg_engine_find_session(sg_engine_t *engine, const char *id);
const sg_session_t *sg_engine_find_session_const(const sg_engine_t *engine, const char *id);
sg_order_t *sg_engine_find_order(sg_engine_t *engine, const char *id);
const sg_order_t *sg_engine_find_order_const(const sg_engine_t *engine, const char *id);
sg_status_t sg_engine_open_session(
    sg_engine_t *engine,
    const char *session_id,
    const char *owner,
    sg_error_t *err);
sg_status_t sg_engine_enqueue_redeem(
    sg_engine_t *engine,
    const char *session_id,
    const char *owner,
    sg_amount_t risk_units,
    sg_amount_t min_cash_out,
    char *out_order_id,
    size_t out_order_id_len,
    sg_error_t *err);
sg_status_t sg_engine_execute_order(
    sg_engine_t *engine,
    const char *order_id,
    sg_error_t *err);
sg_status_t sg_engine_execute_next(
    sg_engine_t *engine,
    const char *session_id,
    sg_error_t *err);
sg_status_t sg_engine_cancel_order(
    sg_engine_t *engine,
    const char *order_id,
    sg_error_t *err);
sg_status_t sg_engine_pause(sg_engine_t *engine, const char *reason, sg_error_t *err);
sg_status_t sg_engine_resume(sg_engine_t *engine, const char *reason, sg_error_t *err);
sg_status_t sg_engine_enter_protection(
    sg_engine_t *engine,
    const char *reason,
    sg_error_t *err);
sg_status_t sg_engine_add_vault_reserve(
    sg_engine_t *engine,
    sg_amount_t amount,
    const char *reason,
    sg_error_t *err);
sg_status_t sg_engine_transfer_cash(
    sg_engine_t *engine,
    const char *from,
    const char *to,
    sg_amount_t amount,
    const char *memo,
    sg_error_t *err);
sg_status_t sg_engine_contribute_to_vault(
    sg_engine_t *engine,
    const char *from,
    sg_amount_t amount,
    const char *reason,
    sg_error_t *err);
sg_status_t sg_engine_withdraw_vault_buffer(
    sg_engine_t *engine,
    const char *to,
    sg_amount_t amount,
    const char *reason,
    sg_error_t *err);
sg_status_t sg_engine_set_vault_haircut(
    sg_engine_t *engine,
    uint32_t haircut_bps,
    const char *reason,
    sg_error_t *err);
sg_status_t sg_engine_risk_snapshot(
    const sg_engine_t *engine,
    sg_risk_snapshot_t *snapshot,
    sg_error_t *err);
bool sg_engine_cash_conservation_ok(const sg_engine_t *engine);
bool sg_engine_unit_conservation_ok(const sg_engine_t *engine);
sg_amount_t sg_engine_total_account_cash(const sg_engine_t *engine);
sg_amount_t sg_engine_total_account_units(const sg_engine_t *engine);

#endif
