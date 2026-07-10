#ifndef SENTINEL_VAULT_H
#define SENTINEL_VAULT_H

#include "sentinel/policy.h"

typedef struct sg_vault {
    sg_id_t id;
    sg_amount_t reserve_cash;
    sg_amount_t issued_risk_units;
    sg_amount_t blocked_cash;
    sg_amount_t reserve_floor;
    sg_amount_t last_unit_value;
    sg_version_t version;
    uint32_t haircut_bps;
} sg_vault_t;

void sg_vault_init(
    sg_vault_t *vault,
    const char *id,
    sg_amount_t reserve_cash,
    sg_amount_t issued_units,
    sg_amount_t reserve_floor);
sg_status_t sg_vault_unit_value(
    const sg_vault_t *vault,
    sg_amount_t *out,
    sg_error_t *err);
sg_status_t sg_vault_preview_redeem(
    const sg_vault_t *vault,
    sg_amount_t risk_units,
    sg_amount_t *out,
    sg_error_t *err);
sg_status_t sg_vault_redeem(
    sg_vault_t *vault,
    sg_amount_t risk_units,
    sg_amount_t cash_out,
    sg_error_t *err);
sg_status_t sg_vault_add_reserve(
    sg_vault_t *vault,
    sg_amount_t amount,
    sg_error_t *err);
sg_status_t sg_vault_withdraw_reserve(
    sg_vault_t *vault,
    sg_amount_t amount,
    sg_error_t *err);
sg_status_t sg_vault_set_haircut(
    sg_vault_t *vault,
    uint32_t haircut_bps,
    sg_error_t *err);
sg_status_t sg_vault_block_cash(
    sg_vault_t *vault,
    sg_amount_t amount,
    sg_error_t *err);
sg_status_t sg_vault_release_blocked(
    sg_vault_t *vault,
    sg_amount_t amount,
    sg_error_t *err);
bool sg_vault_reserve_floor_ok(const sg_vault_t *vault);
sg_amount_t sg_vault_liquid_cash(const sg_vault_t *vault);

#endif
