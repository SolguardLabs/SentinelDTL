#include "sentinel/policy.h"

void sg_policy_default(sg_policy_t *policy) {
    if (policy == NULL) {
        return;
    }
    policy->session_limit = 180000;
    policy->single_order_limit = 90000;
    policy->queue_limit = 140000;
    policy->reserve_floor = 250000;
    policy->execution_fee_bps = 20;
    policy->breaker_surge_bps = 2500;
    policy->breaker_drop_bps = 2500;
    policy->max_orders_per_session = 8;
    policy->execute_during_protection = true;
    policy->allow_operator_reprice = true;
}

sg_status_t sg_policy_validate(const sg_policy_t *policy, sg_error_t *err) {
    if (policy == NULL) {
        sg_error_set(err, SG_ERR_INVALID_ARGUMENT, "policy is required");
        return SG_ERR_INVALID_ARGUMENT;
    }
    if (policy->session_limit == 0 || policy->single_order_limit == 0) {
        sg_error_set(err, SG_ERR_INVALID_ARGUMENT, "policy limits must be non-zero");
        return SG_ERR_INVALID_ARGUMENT;
    }
    if (policy->single_order_limit > policy->session_limit) {
        sg_error_set(err, SG_ERR_INVALID_ARGUMENT, "single order limit exceeds session limit");
        return SG_ERR_INVALID_ARGUMENT;
    }
    if (policy->queue_limit > policy->session_limit) {
        sg_error_set(err, SG_ERR_INVALID_ARGUMENT, "queue limit exceeds session limit");
        return SG_ERR_INVALID_ARGUMENT;
    }
    if (policy->execution_fee_bps > SG_BPS_DENOMINATOR) {
        sg_error_set(err, SG_ERR_INVALID_ARGUMENT, "fee bps exceeds denominator");
        return SG_ERR_INVALID_ARGUMENT;
    }
    if (policy->max_orders_per_session == 0) {
        sg_error_set(err, SG_ERR_INVALID_ARGUMENT, "session order count must be non-zero");
        return SG_ERR_INVALID_ARGUMENT;
    }
    return SG_OK;
}

sg_status_t sg_policy_check_order(
    const sg_policy_t *policy,
    sg_amount_t order_notional,
    sg_amount_t queued_notional,
    uint32_t queued_orders,
    sg_error_t *err) {
    sg_amount_t next_queue = 0;
    sg_status_t status = sg_policy_validate(policy, err);
    if (status != SG_OK) {
        return status;
    }
    if (order_notional == 0) {
        sg_error_set(err, SG_ERR_INVALID_ARGUMENT, "order notional must be non-zero");
        return SG_ERR_INVALID_ARGUMENT;
    }
    if (order_notional > policy->single_order_limit) {
        sg_error_set(err, SG_ERR_LIMIT, "order exceeds single order limit");
        return SG_ERR_LIMIT;
    }
    if (queued_orders >= policy->max_orders_per_session) {
        sg_error_set(err, SG_ERR_LIMIT, "session order count limit reached");
        return SG_ERR_LIMIT;
    }
    if (!sg_amount_add(queued_notional, order_notional, &next_queue)) {
        sg_error_set(err, SG_ERR_OVERFLOW, "queue notional overflow");
        return SG_ERR_OVERFLOW;
    }
    if (next_queue > policy->queue_limit) {
        sg_error_set(err, SG_ERR_LIMIT, "queue notional limit reached");
        return SG_ERR_LIMIT;
    }
    return SG_OK;
}

bool sg_policy_trip_surge(
    const sg_policy_t *policy,
    sg_amount_t previous_value,
    sg_amount_t next_value) {
    sg_amount_t delta = 0;
    sg_amount_t threshold = 0;
    if (policy == NULL || previous_value == 0 || next_value <= previous_value) {
        return false;
    }
    delta = next_value - previous_value;
    if (!sg_amount_mul_bps(previous_value, policy->breaker_surge_bps, &threshold)) {
        return true;
    }
    return delta >= threshold;
}

bool sg_policy_trip_drop(
    const sg_policy_t *policy,
    sg_amount_t previous_value,
    sg_amount_t next_value) {
    sg_amount_t delta = 0;
    sg_amount_t threshold = 0;
    if (policy == NULL || previous_value == 0 || next_value >= previous_value) {
        return false;
    }
    delta = previous_value - next_value;
    if (!sg_amount_mul_bps(previous_value, policy->breaker_drop_bps, &threshold)) {
        return true;
    }
    return delta >= threshold;
}
