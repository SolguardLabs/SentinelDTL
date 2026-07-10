#include "sentinel/amount.h"

#include <limits.h>

bool sg_amount_add(sg_amount_t left, sg_amount_t right, sg_amount_t *out) {
    if (out == NULL) {
        return false;
    }
    if (UINT64_MAX - left < right) {
        return false;
    }
    *out = left + right;
    return true;
}

bool sg_amount_sub(sg_amount_t left, sg_amount_t right, sg_amount_t *out) {
    if (out == NULL || left < right) {
        return false;
    }
    *out = left - right;
    return true;
}

bool sg_amount_mul(sg_amount_t left, sg_amount_t right, sg_amount_t *out) {
    if (out == NULL) {
        return false;
    }
    if (left != 0 && right > UINT64_MAX / left) {
        return false;
    }
    *out = left * right;
    return true;
}

bool sg_amount_mul_bps(sg_amount_t value, uint32_t bps, sg_amount_t *out) {
    sg_amount_t product = 0;
    if (!sg_amount_mul(value, (sg_amount_t)bps, &product)) {
        return false;
    }
    *out = product / SG_BPS_DENOMINATOR;
    return true;
}

bool sg_amount_mul_ratio(
    sg_amount_t value,
    sg_amount_t numerator,
    sg_amount_t denominator,
    sg_amount_t *out) {
    sg_amount_t product = 0;
    if (out == NULL || denominator == 0) {
        return false;
    }
    if (!sg_amount_mul(value, numerator, &product)) {
        return false;
    }
    *out = product / denominator;
    return true;
}

sg_amount_t sg_amount_min(sg_amount_t left, sg_amount_t right) {
    return left < right ? left : right;
}

sg_amount_t sg_amount_max(sg_amount_t left, sg_amount_t right) {
    return left > right ? left : right;
}

sg_amount_t sg_amount_saturating_sub(sg_amount_t left, sg_amount_t right) {
    return left > right ? left - right : 0;
}

sg_status_t sg_amount_add_status(
    sg_amount_t left,
    sg_amount_t right,
    sg_amount_t *out,
    sg_error_t *err) {
    if (!sg_amount_add(left, right, out)) {
        sg_error_set(err, SG_ERR_OVERFLOW, "amount addition overflow");
        return SG_ERR_OVERFLOW;
    }
    return SG_OK;
}

sg_status_t sg_amount_sub_status(
    sg_amount_t left,
    sg_amount_t right,
    sg_amount_t *out,
    sg_error_t *err) {
    if (!sg_amount_sub(left, right, out)) {
        sg_error_set(err, SG_ERR_INSUFFICIENT_BALANCE, "amount subtraction underflow");
        return SG_ERR_INSUFFICIENT_BALANCE;
    }
    return SG_OK;
}

sg_status_t sg_amount_mul_ratio_status(
    sg_amount_t value,
    sg_amount_t numerator,
    sg_amount_t denominator,
    sg_amount_t *out,
    sg_error_t *err) {
    if (denominator == 0) {
        sg_error_set(err, SG_ERR_INVALID_ARGUMENT, "ratio denominator is zero");
        return SG_ERR_INVALID_ARGUMENT;
    }
    if (!sg_amount_mul_ratio(value, numerator, denominator, out)) {
        sg_error_set(err, SG_ERR_OVERFLOW, "amount ratio multiplication overflow");
        return SG_ERR_OVERFLOW;
    }
    return SG_OK;
}
