#include "sentinel/engine.h"

#include <string.h>

static uint32_t sg_risk_ratio_bps(sg_amount_t value, sg_amount_t limit) {
    sg_amount_t scaled = 0;
    sg_amount_t ratio = 0;
    if (limit == 0 || value == 0) {
        return 0;
    }
    if (!sg_amount_mul(value, SG_BPS_DENOMINATOR, &scaled)) {
        return UINT32_MAX;
    }
    ratio = scaled / limit;
    if (ratio > UINT32_MAX) {
        return UINT32_MAX;
    }
    return (uint32_t)ratio;
}

void sg_risk_snapshot_clear(sg_risk_snapshot_t *snapshot) {
    if (snapshot == NULL) {
        return;
    }
    memset(snapshot, 0, sizeof(*snapshot));
}

sg_status_t sg_risk_snapshot_from_engine(
    const struct sg_engine *engine,
    sg_risk_snapshot_t *snapshot,
    sg_error_t *err) {
    sg_amount_t session_capacity = 0;
    sg_amount_t session_accounted = 0;
    sg_amount_t total_cash = 0;
    size_t index = 0;
    if (engine == NULL || snapshot == NULL) {
        sg_error_set(err, SG_ERR_INVALID_ARGUMENT, "engine and snapshot are required");
        return SG_ERR_INVALID_ARGUMENT;
    }
    sg_risk_snapshot_clear(snapshot);
    snapshot->account_cash = sg_engine_total_account_cash(engine);
    snapshot->vault_cash = engine->vault.reserve_cash;
    snapshot->vault_liquid_cash = sg_vault_liquid_cash(&engine->vault);
    snapshot->reserve_headroom =
        sg_amount_saturating_sub(engine->vault.reserve_cash, engine->vault.reserve_floor);
    snapshot->total_risk_units = sg_engine_total_account_units(engine);
    snapshot->reserve_floor_ok = sg_vault_reserve_floor_ok(&engine->vault);
    snapshot->cash_conservation_ok = sg_engine_cash_conservation_ok(engine);
    snapshot->unit_conservation_ok = sg_engine_unit_conservation_ok(engine);
    if (!sg_amount_add(snapshot->account_cash, snapshot->vault_cash, &total_cash)) {
        sg_error_set(err, SG_ERR_OVERFLOW, "risk cash total overflow");
        return SG_ERR_OVERFLOW;
    }
    snapshot->total_cash = total_cash;
    for (index = 0; index < engine->account_len; index++) {
        if (!sg_amount_add(
                snapshot->queued_risk_units,
                engine->accounts[index].queued_units,
                &snapshot->queued_risk_units)) {
            sg_error_set(err, SG_ERR_OVERFLOW, "risk queued unit overflow");
            return SG_ERR_OVERFLOW;
        }
    }
    for (index = 0; index < engine->session_len; index++) {
        const sg_session_t *session = &engine->sessions[index];
        if (session->state == SG_SESSION_OPEN) {
            snapshot->open_sessions += 1;
        }
        if (session->state == SG_SESSION_PROTECTION) {
            snapshot->protected_sessions += 1;
        }
        if (!sg_amount_add(session_accounted, sg_session_accounted_notional(session), &session_accounted)) {
            sg_error_set(err, SG_ERR_OVERFLOW, "risk session notional overflow");
            return SG_ERR_OVERFLOW;
        }
    }
    for (index = 0; index < engine->order_len; index++) {
        const sg_order_t *order = &engine->orders[index];
        if (!sg_order_is_live(order)) {
            continue;
        }
        snapshot->live_orders += 1;
        if (!sg_amount_add(
                snapshot->live_order_notional,
                order->admission_notional,
                &snapshot->live_order_notional)) {
            sg_error_set(err, SG_ERR_OVERFLOW, "risk live order notional overflow");
            return SG_ERR_OVERFLOW;
        }
        if (!sg_amount_add(snapshot->live_order_units, order->risk_units, &snapshot->live_order_units)) {
            sg_error_set(err, SG_ERR_OVERFLOW, "risk live order unit overflow");
            return SG_ERR_OVERFLOW;
        }
    }
    if (engine->session_len > 0 &&
        sg_amount_mul(engine->policy.session_limit, (sg_amount_t)engine->session_len, &session_capacity)) {
        snapshot->session_utilization_bps = sg_risk_ratio_bps(session_accounted, session_capacity);
    }
    snapshot->queue_utilization_bps =
        sg_risk_ratio_bps(snapshot->live_order_notional, engine->policy.queue_limit);
    snapshot->reserve_floor_bps =
        sg_risk_ratio_bps(engine->vault.reserve_floor, engine->vault.reserve_cash);
    return SG_OK;
}

bool sg_risk_snapshot_is_balanced(const sg_risk_snapshot_t *snapshot) {
    if (snapshot == NULL) {
        return false;
    }
    return snapshot->reserve_floor_ok &&
           snapshot->cash_conservation_ok &&
           snapshot->unit_conservation_ok;
}

const char *sg_risk_band_name(uint32_t utilization_bps) {
    if (utilization_bps >= 9000) {
        return "critical";
    }
    if (utilization_bps >= 7000) {
        return "elevated";
    }
    if (utilization_bps >= 4000) {
        return "watch";
    }
    return "normal";
}
