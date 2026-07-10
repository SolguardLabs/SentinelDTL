#include "sentinel/engine.h"

#include <stdio.h>
#include <string.h>

static sg_time_t sg_engine_tick(sg_engine_t *engine) {
    if (engine == NULL) {
        return 0;
    }
    engine->clock += 1;
    return engine->clock;
}

static bool sg_account_available_units(
    const sg_account_t *account,
    sg_amount_t *out) {
    if (account == NULL || out == NULL || account->risk_units < account->queued_units) {
        return false;
    }
    *out = account->risk_units - account->queued_units;
    return true;
}

static sg_status_t sg_engine_append_expected_cash(
    sg_engine_t *engine,
    sg_amount_t amount,
    sg_error_t *err) {
    sg_amount_t next = 0;
    if (!sg_amount_add(engine->expected_cash_supply, amount, &next)) {
        sg_error_set(err, SG_ERR_OVERFLOW, "cash supply overflow");
        return SG_ERR_OVERFLOW;
    }
    engine->expected_cash_supply = next;
    return SG_OK;
}

static bool sg_owner_matches(const sg_session_t *session, const sg_account_t *account) {
    if (session == NULL || account == NULL) {
        return false;
    }
    return sg_id_eq(&session->owner, &account->id);
}

static sg_status_t sg_account_debit_cash(
    sg_account_t *account,
    sg_amount_t amount,
    sg_error_t *err) {
    sg_amount_t next_debited = 0;
    if (account == NULL || amount == 0) {
        sg_error_set(err, SG_ERR_INVALID_ARGUMENT, "cash debit requires an account and amount");
        return SG_ERR_INVALID_ARGUMENT;
    }
    if (account->cash_balance < amount) {
        sg_error_set(err, SG_ERR_INSUFFICIENT_BALANCE, "account cash is insufficient");
        return SG_ERR_INSUFFICIENT_BALANCE;
    }
    if (!sg_amount_add(account->debited_cash, amount, &next_debited)) {
        sg_error_set(err, SG_ERR_OVERFLOW, "account debit total overflow");
        return SG_ERR_OVERFLOW;
    }
    account->cash_balance -= amount;
    account->debited_cash = next_debited;
    return SG_OK;
}

static sg_status_t sg_account_credit_cash(
    sg_account_t *account,
    sg_amount_t amount,
    sg_error_t *err) {
    sg_amount_t next_balance = 0;
    sg_amount_t next_credited = 0;
    if (account == NULL || amount == 0) {
        sg_error_set(err, SG_ERR_INVALID_ARGUMENT, "cash credit requires an account and amount");
        return SG_ERR_INVALID_ARGUMENT;
    }
    if (!sg_amount_add(account->cash_balance, amount, &next_balance) ||
        !sg_amount_add(account->credited_cash, amount, &next_credited)) {
        sg_error_set(err, SG_ERR_OVERFLOW, "account credit overflow");
        return SG_ERR_OVERFLOW;
    }
    account->cash_balance = next_balance;
    account->credited_cash = next_credited;
    return SG_OK;
}

void sg_engine_init(sg_engine_t *engine) {
    size_t index = 0;
    if (engine == NULL) {
        return;
    }
    memset(engine, 0, sizeof(*engine));
    sg_policy_default(&engine->policy);
    sg_vault_init(
        &engine->vault,
        "vault-main",
        900000,
        9000,
        engine->policy.reserve_floor);
    for (index = 0; index < SG_MAX_ORDERS; index++) {
        sg_order_clear(&engine->orders[index]);
    }
    sg_event_log_init(&engine->events);
    engine->mode = SG_CONTROL_ACTIVE;
    engine->clock = 0;
    engine->expected_cash_supply = engine->vault.reserve_cash;
    engine->account_len = 0;
    engine->session_len = 0;
    engine->order_len = 0;
    engine->next_order_seq = 1;
}

sg_status_t sg_engine_bootstrap(sg_engine_t *engine, sg_error_t *err) {
    sg_status_t status = SG_OK;
    if (engine == NULL) {
        sg_error_set(err, SG_ERR_INVALID_ARGUMENT, "engine is required");
        return SG_ERR_INVALID_ARGUMENT;
    }
    sg_engine_init(engine);
    status = sg_policy_validate(&engine->policy, err);
    if (status != SG_OK) {
        return status;
    }
    return sg_event_push(
        &engine->events,
        SG_EVENT_BOOT,
        sg_engine_tick(engine),
        engine->vault.id.value,
        engine->vault.reserve_cash,
        engine->vault.issued_risk_units,
        engine->vault.version,
        "engine bootstrapped",
        err);
}

sg_status_t sg_engine_add_account(
    sg_engine_t *engine,
    const char *id,
    sg_amount_t cash_balance,
    sg_amount_t risk_units,
    sg_error_t *err) {
    sg_account_t *account = NULL;
    sg_status_t status = SG_OK;
    if (engine == NULL || id == NULL || id[0] == '\0') {
        sg_error_set(err, SG_ERR_INVALID_ARGUMENT, "account id is required");
        return SG_ERR_INVALID_ARGUMENT;
    }
    if (sg_engine_find_account(engine, id) != NULL) {
        sg_error_set(err, SG_ERR_DUPLICATE, "account already exists");
        return SG_ERR_DUPLICATE;
    }
    if (engine->account_len >= SG_MAX_ACCOUNTS) {
        sg_error_set(err, SG_ERR_CAPACITY, "account capacity reached");
        return SG_ERR_CAPACITY;
    }
    status = sg_engine_append_expected_cash(engine, cash_balance, err);
    if (status != SG_OK) {
        return status;
    }
    account = &engine->accounts[engine->account_len++];
    sg_id_from_text(&account->id, id);
    account->cash_balance = cash_balance;
    account->risk_units = risk_units;
    account->queued_units = 0;
    account->credited_cash = 0;
    account->debited_cash = 0;
    return sg_event_push(
        &engine->events,
        SG_EVENT_ACCOUNT_CREATED,
        sg_engine_tick(engine),
        id,
        cash_balance,
        risk_units,
        engine->vault.version,
        "account created",
        err);
}

sg_account_t *sg_engine_find_account(sg_engine_t *engine, const char *id) {
    size_t index = 0;
    if (engine == NULL || id == NULL) {
        return NULL;
    }
    for (index = 0; index < engine->account_len; index++) {
        if (strncmp(engine->accounts[index].id.value, id, SG_ID_LEN) == 0) {
            return &engine->accounts[index];
        }
    }
    return NULL;
}

const sg_account_t *sg_engine_find_account_const(const sg_engine_t *engine, const char *id) {
    size_t index = 0;
    if (engine == NULL || id == NULL) {
        return NULL;
    }
    for (index = 0; index < engine->account_len; index++) {
        if (strncmp(engine->accounts[index].id.value, id, SG_ID_LEN) == 0) {
            return &engine->accounts[index];
        }
    }
    return NULL;
}

sg_session_t *sg_engine_find_session(sg_engine_t *engine, const char *id) {
    size_t index = 0;
    if (engine == NULL || id == NULL) {
        return NULL;
    }
    for (index = 0; index < engine->session_len; index++) {
        if (strncmp(engine->sessions[index].id.value, id, SG_ID_LEN) == 0) {
            return &engine->sessions[index];
        }
    }
    return NULL;
}

const sg_session_t *sg_engine_find_session_const(const sg_engine_t *engine, const char *id) {
    size_t index = 0;
    if (engine == NULL || id == NULL) {
        return NULL;
    }
    for (index = 0; index < engine->session_len; index++) {
        if (strncmp(engine->sessions[index].id.value, id, SG_ID_LEN) == 0) {
            return &engine->sessions[index];
        }
    }
    return NULL;
}

sg_order_t *sg_engine_find_order(sg_engine_t *engine, const char *id) {
    size_t index = 0;
    if (engine == NULL || id == NULL) {
        return NULL;
    }
    for (index = 0; index < engine->order_len; index++) {
        if (strncmp(engine->orders[index].id.value, id, SG_ID_LEN) == 0) {
            return &engine->orders[index];
        }
    }
    return NULL;
}

const sg_order_t *sg_engine_find_order_const(const sg_engine_t *engine, const char *id) {
    size_t index = 0;
    if (engine == NULL || id == NULL) {
        return NULL;
    }
    for (index = 0; index < engine->order_len; index++) {
        if (strncmp(engine->orders[index].id.value, id, SG_ID_LEN) == 0) {
            return &engine->orders[index];
        }
    }
    return NULL;
}

sg_status_t sg_engine_open_session(
    sg_engine_t *engine,
    const char *session_id,
    const char *owner,
    sg_error_t *err) {
    sg_session_t *session = NULL;
    if (engine == NULL || session_id == NULL || owner == NULL) {
        sg_error_set(err, SG_ERR_INVALID_ARGUMENT, "session id and owner are required");
        return SG_ERR_INVALID_ARGUMENT;
    }
    if (sg_engine_find_account(engine, owner) == NULL) {
        sg_error_set(err, SG_ERR_NOT_FOUND, "session owner does not exist");
        return SG_ERR_NOT_FOUND;
    }
    if (sg_engine_find_session(engine, session_id) != NULL) {
        sg_error_set(err, SG_ERR_DUPLICATE, "session already exists");
        return SG_ERR_DUPLICATE;
    }
    if (engine->session_len >= SG_MAX_SESSIONS) {
        sg_error_set(err, SG_ERR_CAPACITY, "session capacity reached");
        return SG_ERR_CAPACITY;
    }
    session = &engine->sessions[engine->session_len++];
    sg_session_init(session, session_id, owner, sg_engine_tick(engine));
    return sg_event_push(
        &engine->events,
        SG_EVENT_SESSION_OPENED,
        engine->clock,
        session_id,
        engine->policy.session_limit,
        0,
        engine->vault.version,
        "session opened",
        err);
}

sg_status_t sg_engine_enqueue_redeem(
    sg_engine_t *engine,
    const char *session_id,
    const char *owner,
    sg_amount_t risk_units,
    sg_amount_t min_cash_out,
    char *out_order_id,
    size_t out_order_id_len,
    sg_error_t *err) {
    sg_session_t *session = NULL;
    sg_account_t *account = NULL;
    sg_order_t *order = NULL;
    sg_amount_t available_units = 0;
    sg_amount_t admission_notional = 0;
    sg_status_t status = SG_OK;
    char order_id[SG_ID_LEN];
    if (engine == NULL || session_id == NULL || owner == NULL) {
        sg_error_set(err, SG_ERR_INVALID_ARGUMENT, "engine, session and owner are required");
        return SG_ERR_INVALID_ARGUMENT;
    }
    if (engine->mode == SG_CONTROL_PAUSED) {
        sg_error_set(err, SG_ERR_PAUSED, "control plane is paused");
        return SG_ERR_PAUSED;
    }
    if (engine->mode == SG_CONTROL_PROTECTION) {
        sg_error_set(err, SG_ERR_PROTECTION, "control plane is in protection");
        return SG_ERR_PROTECTION;
    }
    session = sg_engine_find_session(engine, session_id);
    account = sg_engine_find_account(engine, owner);
    if (session == NULL || account == NULL) {
        sg_error_set(err, SG_ERR_NOT_FOUND, "session or account was not found");
        return SG_ERR_NOT_FOUND;
    }
    if (!sg_owner_matches(session, account)) {
        sg_error_set(err, SG_ERR_INVALID_ARGUMENT, "session owner mismatch");
        return SG_ERR_INVALID_ARGUMENT;
    }
    if (!sg_account_available_units(account, &available_units) || risk_units > available_units) {
        sg_session_mark_rejection(session, 0, sg_engine_tick(engine));
        sg_error_set(err, SG_ERR_INSUFFICIENT_BALANCE, "available risk units are insufficient");
        return SG_ERR_INSUFFICIENT_BALANCE;
    }
    status = sg_vault_preview_redeem(&engine->vault, risk_units, &admission_notional, err);
    if (status != SG_OK) {
        sg_session_mark_rejection(session, 0, sg_engine_tick(engine));
        return status;
    }
    status = sg_session_reserve_order(
        session,
        &engine->policy,
        admission_notional,
        sg_engine_tick(engine),
        err);
    if (status != SG_OK) {
        sg_session_mark_rejection(session, admission_notional, engine->clock);
        (void)sg_event_push(
            &engine->events,
            SG_EVENT_ORDER_REJECTED,
            engine->clock,
            session_id,
            admission_notional,
            risk_units,
            engine->vault.version,
            sg_status_name(status),
            NULL);
        return status;
    }
    if (engine->order_len >= SG_MAX_ORDERS) {
        (void)sg_session_cancel_order(session, admission_notional, engine->clock, NULL);
        sg_error_set(err, SG_ERR_CAPACITY, "order capacity reached");
        return SG_ERR_CAPACITY;
    }
    snprintf(order_id, sizeof(order_id), "ord-%06llu", (unsigned long long)engine->next_order_seq++);
    order = &engine->orders[engine->order_len++];
    sg_order_init_redeem(
        order,
        order_id,
        session_id,
        owner,
        risk_units,
        admission_notional,
        min_cash_out,
        engine->vault.version,
        engine->clock);
    account->queued_units += risk_units;
    if (out_order_id != NULL && out_order_id_len > 0) {
        strncpy(out_order_id, order_id, out_order_id_len - 1);
        out_order_id[out_order_id_len - 1] = '\0';
    }
    return sg_event_push(
        &engine->events,
        SG_EVENT_ORDER_QUEUED,
        engine->clock,
        order_id,
        admission_notional,
        risk_units,
        engine->vault.version,
        "order queued",
        err);
}

sg_status_t sg_engine_execute_order(
    sg_engine_t *engine,
    const char *order_id,
    sg_error_t *err) {
    sg_order_t *order = NULL;
    sg_session_t *session = NULL;
    sg_account_t *account = NULL;
    sg_amount_t gross_cash_out = 0;
    sg_amount_t fee = 0;
    sg_amount_t net_cash_out = 0;
    sg_amount_t next_cash = 0;
    sg_status_t status = SG_OK;
    if (engine == NULL || order_id == NULL) {
        sg_error_set(err, SG_ERR_INVALID_ARGUMENT, "engine and order id are required");
        return SG_ERR_INVALID_ARGUMENT;
    }
    if (engine->mode == SG_CONTROL_PAUSED) {
        sg_error_set(err, SG_ERR_PAUSED, "control plane is paused");
        return SG_ERR_PAUSED;
    }
    order = sg_engine_find_order(engine, order_id);
    if (order == NULL) {
        sg_error_set(err, SG_ERR_NOT_FOUND, "order was not found");
        return SG_ERR_NOT_FOUND;
    }
    if (!sg_order_is_live(order)) {
        sg_error_set(err, SG_ERR_ORDER_STATE, "order is not queued");
        return SG_ERR_ORDER_STATE;
    }
    session = sg_engine_find_session(engine, order->session_id.value);
    account = sg_engine_find_account(engine, order->owner.value);
    if (session == NULL || account == NULL) {
        sg_error_set(err, SG_ERR_NOT_FOUND, "session or account was not found");
        return SG_ERR_NOT_FOUND;
    }
    if (account->queued_units < order->risk_units || account->risk_units < order->risk_units) {
        sg_error_set(err, SG_ERR_INSUFFICIENT_BALANCE, "queued risk units are inconsistent");
        return SG_ERR_INSUFFICIENT_BALANCE;
    }
    status = sg_vault_preview_redeem(&engine->vault, order->risk_units, &gross_cash_out, err);
    if (status != SG_OK) {
        return status;
    }
    if (gross_cash_out < order->min_cash_out) {
        sg_error_set(err, SG_ERR_LIMIT, "order minimum cash out was not reached");
        return SG_ERR_LIMIT;
    }
    if (!sg_amount_mul_bps(gross_cash_out, engine->policy.execution_fee_bps, &fee)) {
        sg_error_set(err, SG_ERR_OVERFLOW, "execution fee overflow");
        return SG_ERR_OVERFLOW;
    }
    if (gross_cash_out < fee) {
        sg_error_set(err, SG_ERR_INTERNAL, "execution fee exceeds cash out");
        return SG_ERR_INTERNAL;
    }
    net_cash_out = gross_cash_out - fee;
    if (net_cash_out > sg_vault_liquid_cash(&engine->vault) ||
        engine->vault.reserve_cash - net_cash_out < engine->vault.reserve_floor) {
        sg_error_set(err, SG_ERR_VAULT_STATE, "vault reserve floor would be crossed");
        return SG_ERR_VAULT_STATE;
    }
    status = sg_session_apply_execution(
        session,
        &engine->policy,
        order->admission_notional,
        gross_cash_out,
        sg_engine_tick(engine),
        err);
    if (status != SG_OK) {
        return status;
    }
    status = sg_vault_redeem(&engine->vault, order->risk_units, net_cash_out, err);
    if (status != SG_OK) {
        return status;
    }
    if (!sg_amount_add(account->cash_balance, net_cash_out, &next_cash)) {
        sg_error_set(err, SG_ERR_OVERFLOW, "account cash overflow");
        return SG_ERR_OVERFLOW;
    }
    account->queued_units -= order->risk_units;
    account->risk_units -= order->risk_units;
    account->cash_balance = next_cash;
    account->credited_cash += net_cash_out;
    sg_order_mark_executed(order, gross_cash_out, fee, engine->vault.version, engine->clock);
    return sg_event_push(
        &engine->events,
        SG_EVENT_ORDER_EXECUTED,
        engine->clock,
        order_id,
        gross_cash_out,
        fee,
        engine->vault.version,
        "order executed",
        err);
}

sg_status_t sg_engine_execute_next(
    sg_engine_t *engine,
    const char *session_id,
    sg_error_t *err) {
    size_t index = 0;
    sg_session_t *session = NULL;
    if (engine == NULL || session_id == NULL) {
        sg_error_set(err, SG_ERR_INVALID_ARGUMENT, "engine and session id are required");
        return SG_ERR_INVALID_ARGUMENT;
    }
    session = sg_engine_find_session(engine, session_id);
    if (session == NULL) {
        sg_error_set(err, SG_ERR_NOT_FOUND, "session was not found");
        return SG_ERR_NOT_FOUND;
    }
    for (index = 0; index < engine->order_len; index++) {
        if (sg_order_is_live(&engine->orders[index]) &&
            sg_order_belongs_to_session(&engine->orders[index], &session->id)) {
            return sg_engine_execute_order(engine, engine->orders[index].id.value, err);
        }
    }
    sg_error_set(err, SG_ERR_NOT_FOUND, "no queued order found for session");
    return SG_ERR_NOT_FOUND;
}

sg_status_t sg_engine_cancel_order(
    sg_engine_t *engine,
    const char *order_id,
    sg_error_t *err) {
    sg_order_t *order = NULL;
    sg_session_t *session = NULL;
    sg_account_t *account = NULL;
    sg_status_t status = SG_OK;
    if (engine == NULL || order_id == NULL) {
        sg_error_set(err, SG_ERR_INVALID_ARGUMENT, "engine and order id are required");
        return SG_ERR_INVALID_ARGUMENT;
    }
    order = sg_engine_find_order(engine, order_id);
    if (order == NULL) {
        sg_error_set(err, SG_ERR_NOT_FOUND, "order was not found");
        return SG_ERR_NOT_FOUND;
    }
    if (!sg_order_is_live(order)) {
        sg_error_set(err, SG_ERR_ORDER_STATE, "order is not queued");
        return SG_ERR_ORDER_STATE;
    }
    session = sg_engine_find_session(engine, order->session_id.value);
    account = sg_engine_find_account(engine, order->owner.value);
    if (session == NULL || account == NULL) {
        sg_error_set(err, SG_ERR_NOT_FOUND, "session or account was not found");
        return SG_ERR_NOT_FOUND;
    }
    status = sg_session_cancel_order(
        session,
        order->admission_notional,
        sg_engine_tick(engine),
        err);
    if (status != SG_OK) {
        return status;
    }
    if (account->queued_units >= order->risk_units) {
        account->queued_units -= order->risk_units;
    }
    sg_order_mark_cancelled(order, engine->clock);
    return sg_event_push(
        &engine->events,
        SG_EVENT_ORDER_CANCELLED,
        engine->clock,
        order_id,
        order->admission_notional,
        order->risk_units,
        engine->vault.version,
        "order cancelled",
        err);
}

sg_status_t sg_engine_pause(sg_engine_t *engine, const char *reason, sg_error_t *err) {
    size_t index = 0;
    if (engine == NULL) {
        sg_error_set(err, SG_ERR_INVALID_ARGUMENT, "engine is required");
        return SG_ERR_INVALID_ARGUMENT;
    }
    if (engine->mode == SG_CONTROL_PAUSED) {
        return SG_OK;
    }
    engine->mode = SG_CONTROL_PAUSED;
    for (index = 0; index < engine->session_len; index++) {
        sg_session_pause(&engine->sessions[index], sg_engine_tick(engine));
    }
    return sg_event_push(
        &engine->events,
        SG_EVENT_PAUSED,
        engine->clock,
        "control-plane",
        0,
        0,
        engine->vault.version,
        reason == NULL ? "paused" : reason,
        err);
}

sg_status_t sg_engine_resume(sg_engine_t *engine, const char *reason, sg_error_t *err) {
    size_t index = 0;
    if (engine == NULL) {
        sg_error_set(err, SG_ERR_INVALID_ARGUMENT, "engine is required");
        return SG_ERR_INVALID_ARGUMENT;
    }
    if (engine->mode == SG_CONTROL_ACTIVE) {
        return SG_OK;
    }
    engine->mode = SG_CONTROL_ACTIVE;
    for (index = 0; index < engine->session_len; index++) {
        sg_session_resume(&engine->sessions[index], sg_engine_tick(engine));
    }
    return sg_event_push(
        &engine->events,
        SG_EVENT_RESUMED,
        engine->clock,
        "control-plane",
        0,
        0,
        engine->vault.version,
        reason == NULL ? "resumed" : reason,
        err);
}

sg_status_t sg_engine_enter_protection(
    sg_engine_t *engine,
    const char *reason,
    sg_error_t *err) {
    size_t index = 0;
    if (engine == NULL) {
        sg_error_set(err, SG_ERR_INVALID_ARGUMENT, "engine is required");
        return SG_ERR_INVALID_ARGUMENT;
    }
    engine->mode = SG_CONTROL_PROTECTION;
    for (index = 0; index < engine->session_len; index++) {
        sg_session_enter_protection(&engine->sessions[index], sg_engine_tick(engine));
    }
    return sg_event_push(
        &engine->events,
        SG_EVENT_PROTECTION_ENTERED,
        engine->clock,
        "control-plane",
        0,
        0,
        engine->vault.version,
        reason == NULL ? "protection" : reason,
        err);
}

sg_status_t sg_engine_add_vault_reserve(
    sg_engine_t *engine,
    sg_amount_t amount,
    const char *reason,
    sg_error_t *err) {
    sg_status_t status = SG_OK;
    if (engine == NULL) {
        sg_error_set(err, SG_ERR_INVALID_ARGUMENT, "engine is required");
        return SG_ERR_INVALID_ARGUMENT;
    }
    status = sg_vault_add_reserve(&engine->vault, amount, err);
    if (status != SG_OK) {
        return status;
    }
    status = sg_engine_append_expected_cash(engine, amount, err);
    if (status != SG_OK) {
        return status;
    }
    return sg_event_push(
        &engine->events,
        SG_EVENT_VAULT_ADJUSTED,
        sg_engine_tick(engine),
        engine->vault.id.value,
        amount,
        engine->vault.reserve_cash,
        engine->vault.version,
        reason == NULL ? "vault adjusted" : reason,
        err);
}

sg_status_t sg_engine_transfer_cash(
    sg_engine_t *engine,
    const char *from,
    const char *to,
    sg_amount_t amount,
    const char *memo,
    sg_error_t *err) {
    sg_account_t *source = NULL;
    sg_account_t *target = NULL;
    sg_amount_t checked_balance = 0;
    sg_amount_t checked_credited = 0;
    sg_status_t status = SG_OK;
    if (engine == NULL || from == NULL || to == NULL) {
        sg_error_set(err, SG_ERR_INVALID_ARGUMENT, "engine and account ids are required");
        return SG_ERR_INVALID_ARGUMENT;
    }
    if (amount == 0) {
        sg_error_set(err, SG_ERR_INVALID_ARGUMENT, "transfer amount must be non-zero");
        return SG_ERR_INVALID_ARGUMENT;
    }
    source = sg_engine_find_account(engine, from);
    target = sg_engine_find_account(engine, to);
    if (source == NULL || target == NULL) {
        sg_error_set(err, SG_ERR_NOT_FOUND, "transfer account was not found");
        return SG_ERR_NOT_FOUND;
    }
    if (source == target) {
        sg_error_set(err, SG_ERR_INVALID_ARGUMENT, "transfer endpoints must differ");
        return SG_ERR_INVALID_ARGUMENT;
    }
    if (!sg_amount_add(target->cash_balance, amount, &checked_balance) ||
        !sg_amount_add(target->credited_cash, amount, &checked_credited)) {
        sg_error_set(err, SG_ERR_OVERFLOW, "transfer target cash overflow");
        return SG_ERR_OVERFLOW;
    }
    (void)checked_balance;
    (void)checked_credited;
    status = sg_account_debit_cash(source, amount, err);
    if (status != SG_OK) {
        return status;
    }
    status = sg_account_credit_cash(target, amount, err);
    if (status != SG_OK) {
        return status;
    }
    return sg_event_push(
        &engine->events,
        SG_EVENT_CHECKPOINT,
        sg_engine_tick(engine),
        from,
        amount,
        target->cash_balance,
        engine->vault.version,
        memo == NULL ? "cash transfer" : memo,
        err);
}

sg_status_t sg_engine_contribute_to_vault(
    sg_engine_t *engine,
    const char *from,
    sg_amount_t amount,
    const char *reason,
    sg_error_t *err) {
    sg_account_t *account = NULL;
    sg_amount_t checked_reserve = 0;
    sg_status_t status = SG_OK;
    if (engine == NULL || from == NULL) {
        sg_error_set(err, SG_ERR_INVALID_ARGUMENT, "engine and account id are required");
        return SG_ERR_INVALID_ARGUMENT;
    }
    account = sg_engine_find_account(engine, from);
    if (account == NULL) {
        sg_error_set(err, SG_ERR_NOT_FOUND, "contributor account was not found");
        return SG_ERR_NOT_FOUND;
    }
    if (!sg_amount_add(engine->vault.reserve_cash, amount, &checked_reserve)) {
        sg_error_set(err, SG_ERR_OVERFLOW, "vault reserve overflow");
        return SG_ERR_OVERFLOW;
    }
    (void)checked_reserve;
    status = sg_account_debit_cash(account, amount, err);
    if (status != SG_OK) {
        return status;
    }
    status = sg_vault_add_reserve(&engine->vault, amount, err);
    if (status != SG_OK) {
        return status;
    }
    return sg_event_push(
        &engine->events,
        SG_EVENT_VAULT_ADJUSTED,
        sg_engine_tick(engine),
        from,
        amount,
        engine->vault.reserve_cash,
        engine->vault.version,
        reason == NULL ? "vault contribution" : reason,
        err);
}

sg_status_t sg_engine_withdraw_vault_buffer(
    sg_engine_t *engine,
    const char *to,
    sg_amount_t amount,
    const char *reason,
    sg_error_t *err) {
    sg_account_t *account = NULL;
    sg_amount_t checked_balance = 0;
    sg_amount_t checked_credited = 0;
    sg_status_t status = SG_OK;
    if (engine == NULL || to == NULL) {
        sg_error_set(err, SG_ERR_INVALID_ARGUMENT, "engine and account id are required");
        return SG_ERR_INVALID_ARGUMENT;
    }
    account = sg_engine_find_account(engine, to);
    if (account == NULL) {
        sg_error_set(err, SG_ERR_NOT_FOUND, "target account was not found");
        return SG_ERR_NOT_FOUND;
    }
    if (!sg_amount_add(account->cash_balance, amount, &checked_balance) ||
        !sg_amount_add(account->credited_cash, amount, &checked_credited)) {
        sg_error_set(err, SG_ERR_OVERFLOW, "vault withdrawal target cash overflow");
        return SG_ERR_OVERFLOW;
    }
    (void)checked_balance;
    (void)checked_credited;
    status = sg_vault_withdraw_reserve(&engine->vault, amount, err);
    if (status != SG_OK) {
        return status;
    }
    status = sg_account_credit_cash(account, amount, err);
    if (status != SG_OK) {
        return status;
    }
    return sg_event_push(
        &engine->events,
        SG_EVENT_VAULT_ADJUSTED,
        sg_engine_tick(engine),
        to,
        amount,
        engine->vault.reserve_cash,
        engine->vault.version,
        reason == NULL ? "vault buffer withdrawal" : reason,
        err);
}

sg_status_t sg_engine_set_vault_haircut(
    sg_engine_t *engine,
    uint32_t haircut_bps,
    const char *reason,
    sg_error_t *err) {
    sg_status_t status = SG_OK;
    if (engine == NULL) {
        sg_error_set(err, SG_ERR_INVALID_ARGUMENT, "engine is required");
        return SG_ERR_INVALID_ARGUMENT;
    }
    status = sg_vault_set_haircut(&engine->vault, haircut_bps, err);
    if (status != SG_OK) {
        return status;
    }
    return sg_event_push(
        &engine->events,
        SG_EVENT_CHECKPOINT,
        sg_engine_tick(engine),
        engine->vault.id.value,
        haircut_bps,
        engine->vault.reserve_cash,
        engine->vault.version,
        reason == NULL ? "vault haircut updated" : reason,
        err);
}

sg_status_t sg_engine_risk_snapshot(
    const sg_engine_t *engine,
    sg_risk_snapshot_t *snapshot,
    sg_error_t *err) {
    return sg_risk_snapshot_from_engine(engine, snapshot, err);
}

bool sg_engine_cash_conservation_ok(const sg_engine_t *engine) {
    sg_amount_t total = 0;
    if (engine == NULL) {
        return false;
    }
    if (!sg_amount_add(sg_engine_total_account_cash(engine), engine->vault.reserve_cash, &total)) {
        return false;
    }
    return total == engine->expected_cash_supply;
}

bool sg_engine_unit_conservation_ok(const sg_engine_t *engine) {
    if (engine == NULL) {
        return false;
    }
    return sg_engine_total_account_units(engine) == engine->vault.issued_risk_units;
}

sg_amount_t sg_engine_total_account_cash(const sg_engine_t *engine) {
    sg_amount_t total = 0;
    size_t index = 0;
    if (engine == NULL) {
        return 0;
    }
    for (index = 0; index < engine->account_len; index++) {
        if (!sg_amount_add(total, engine->accounts[index].cash_balance, &total)) {
            return UINT64_MAX;
        }
    }
    return total;
}

sg_amount_t sg_engine_total_account_units(const sg_engine_t *engine) {
    sg_amount_t total = 0;
    size_t index = 0;
    if (engine == NULL) {
        return 0;
    }
    for (index = 0; index < engine->account_len; index++) {
        if (!sg_amount_add(total, engine->accounts[index].risk_units, &total)) {
            return UINT64_MAX;
        }
    }
    return total;
}
