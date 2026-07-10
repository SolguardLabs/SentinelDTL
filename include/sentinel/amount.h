#ifndef SENTINEL_AMOUNT_H
#define SENTINEL_AMOUNT_H

#include "sentinel/error.h"

#define SG_BPS_DENOMINATOR 10000ULL

bool sg_amount_add(sg_amount_t left, sg_amount_t right, sg_amount_t *out);
bool sg_amount_sub(sg_amount_t left, sg_amount_t right, sg_amount_t *out);
bool sg_amount_mul(sg_amount_t left, sg_amount_t right, sg_amount_t *out);
bool sg_amount_mul_bps(sg_amount_t value, uint32_t bps, sg_amount_t *out);
bool sg_amount_mul_ratio(
    sg_amount_t value,
    sg_amount_t numerator,
    sg_amount_t denominator,
    sg_amount_t *out);
sg_amount_t sg_amount_min(sg_amount_t left, sg_amount_t right);
sg_amount_t sg_amount_max(sg_amount_t left, sg_amount_t right);
sg_amount_t sg_amount_saturating_sub(sg_amount_t left, sg_amount_t right);
sg_status_t sg_amount_add_status(
    sg_amount_t left,
    sg_amount_t right,
    sg_amount_t *out,
    sg_error_t *err);
sg_status_t sg_amount_sub_status(
    sg_amount_t left,
    sg_amount_t right,
    sg_amount_t *out,
    sg_error_t *err);
sg_status_t sg_amount_mul_ratio_status(
    sg_amount_t value,
    sg_amount_t numerator,
    sg_amount_t denominator,
    sg_amount_t *out,
    sg_error_t *err);

#endif
