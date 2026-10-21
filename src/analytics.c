#include "sentinel/analytics.h"

#include "sentinel/amount.h"

#include <string.h>

sg_status_t sg_liquidity_envelope_from_vault(
    const sg_vault_t *vault,
    uint32_t stress_bps,
    sg_liquidity_envelope_t *out,
    sg_error_t *err) {
    sg_amount_t stressed_liquid = 0;
    if (vault == NULL || out == NULL) {
        sg_error_set(err, SG_ERR_INVALID_ARGUMENT, "vault and output are required");
        return SG_ERR_INVALID_ARGUMENT;
    }
    if (stress_bps > SG_BPS_DENOMINATOR) {
        sg_error_set(err, SG_ERR_INVALID_ARGUMENT, "stress bps exceeds denominator");
        return SG_ERR_INVALID_ARGUMENT;
    }
    if (vault->blocked_cash > vault->reserve_cash) {
        sg_error_set(err, SG_ERR_VAULT_STATE, "blocked cash exceeds reserves");
        return SG_ERR_VAULT_STATE;
    }
    memset(out, 0, sizeof(*out));
    out->reserve_cash = vault->reserve_cash;
    out->reserve_floor = vault->reserve_floor;
    out->blocked_cash = vault->blocked_cash;
    out->liquid_cash = sg_vault_liquid_cash(vault);
    out->reserve_headroom = sg_amount_saturating_sub(vault->reserve_cash, vault->reserve_floor);
    out->redeem_capacity = sg_amount_min(out->liquid_cash, out->reserve_headroom);
    out->stress_bps = stress_bps;
    if (!sg_amount_mul_bps(vault->reserve_cash, stress_bps, &out->stress_loss)) {
        sg_error_set(err, SG_ERR_OVERFLOW, "stress calculation overflow");
        return SG_ERR_OVERFLOW;
    }
    out->stressed_reserve = vault->reserve_cash - out->stress_loss;
    out->stressed_headroom = sg_amount_saturating_sub(out->stressed_reserve, vault->reserve_floor);
    stressed_liquid = sg_amount_saturating_sub(out->stressed_reserve, vault->blocked_cash);
    out->stressed_redeem_capacity = sg_amount_min(stressed_liquid, out->stressed_headroom);
    out->floor_covered_after_stress = out->stressed_reserve >= vault->reserve_floor;
    return SG_OK;
}
