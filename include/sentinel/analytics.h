#ifndef SENTINEL_ANALYTICS_H
#define SENTINEL_ANALYTICS_H

#include "sentinel/vault.h"

typedef struct sg_liquidity_envelope {
    sg_amount_t reserve_cash;
    sg_amount_t reserve_floor;
    sg_amount_t blocked_cash;
    sg_amount_t liquid_cash;
    sg_amount_t reserve_headroom;
    sg_amount_t redeem_capacity;
    uint32_t stress_bps;
    sg_amount_t stress_loss;
    sg_amount_t stressed_reserve;
    sg_amount_t stressed_headroom;
    sg_amount_t stressed_redeem_capacity;
    bool floor_covered_after_stress;
} sg_liquidity_envelope_t;

sg_status_t sg_liquidity_envelope_from_vault(
    const sg_vault_t *vault,
    uint32_t stress_bps,
    sg_liquidity_envelope_t *out,
    sg_error_t *err);

#endif
