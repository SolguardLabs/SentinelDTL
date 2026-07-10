#ifndef SENTINEL_RISK_H
#define SENTINEL_RISK_H

#include "sentinel/error.h"

struct sg_engine;

typedef struct sg_risk_snapshot {
    sg_amount_t total_cash;
    sg_amount_t account_cash;
    sg_amount_t vault_cash;
    sg_amount_t vault_liquid_cash;
    sg_amount_t reserve_headroom;
    sg_amount_t total_risk_units;
    sg_amount_t queued_risk_units;
    sg_amount_t live_order_notional;
    sg_amount_t live_order_units;
    uint32_t session_utilization_bps;
    uint32_t queue_utilization_bps;
    uint32_t reserve_floor_bps;
    uint32_t open_sessions;
    uint32_t protected_sessions;
    uint32_t live_orders;
    bool reserve_floor_ok;
    bool cash_conservation_ok;
    bool unit_conservation_ok;
} sg_risk_snapshot_t;

void sg_risk_snapshot_clear(sg_risk_snapshot_t *snapshot);
sg_status_t sg_risk_snapshot_from_engine(
    const struct sg_engine *engine,
    sg_risk_snapshot_t *snapshot,
    sg_error_t *err);
bool sg_risk_snapshot_is_balanced(const sg_risk_snapshot_t *snapshot);
const char *sg_risk_band_name(uint32_t utilization_bps);

#endif
