#ifndef SENTINEL_POLICY_H
#define SENTINEL_POLICY_H

#include "sentinel/amount.h"

typedef struct sg_policy {
    sg_amount_t session_limit;
    sg_amount_t single_order_limit;
    sg_amount_t queue_limit;
    sg_amount_t reserve_floor;
    uint32_t execution_fee_bps;
    uint32_t breaker_surge_bps;
    uint32_t breaker_drop_bps;
    uint32_t max_orders_per_session;
    bool execute_during_protection;
    bool allow_operator_reprice;
} sg_policy_t;

void sg_policy_default(sg_policy_t *policy);
sg_status_t sg_policy_validate(const sg_policy_t *policy, sg_error_t *err);
sg_status_t sg_policy_check_order(
    const sg_policy_t *policy,
    sg_amount_t order_notional,
    sg_amount_t queued_notional,
    uint32_t queued_orders,
    sg_error_t *err);
bool sg_policy_trip_surge(
    const sg_policy_t *policy,
    sg_amount_t previous_value,
    sg_amount_t next_value);
bool sg_policy_trip_drop(
    const sg_policy_t *policy,
    sg_amount_t previous_value,
    sg_amount_t next_value);

#endif
