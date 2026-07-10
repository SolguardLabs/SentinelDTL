#include "sentinel/vault.h"

void sg_vault_init(
    sg_vault_t *vault,
    const char *id,
    sg_amount_t reserve_cash,
    sg_amount_t issued_units,
    sg_amount_t reserve_floor) {
    sg_error_t ignored;
    if (vault == NULL) {
        return;
    }
    sg_error_clear(&ignored);
    sg_id_from_text(&vault->id, id);
    vault->reserve_cash = reserve_cash;
    vault->issued_risk_units = issued_units;
    vault->blocked_cash = 0;
    vault->reserve_floor = reserve_floor;
    vault->last_unit_value = 0;
    vault->version = 1;
    vault->haircut_bps = 0;
    (void)sg_vault_unit_value(vault, &vault->last_unit_value, &ignored);
}

sg_status_t sg_vault_unit_value(
    const sg_vault_t *vault,
    sg_amount_t *out,
    sg_error_t *err) {
    if (vault == NULL || out == NULL) {
        sg_error_set(err, SG_ERR_INVALID_ARGUMENT, "vault and output are required");
        return SG_ERR_INVALID_ARGUMENT;
    }
    if (vault->issued_risk_units == 0) {
        sg_error_set(err, SG_ERR_VAULT_STATE, "vault has no issued units");
        return SG_ERR_VAULT_STATE;
    }
    *out = vault->reserve_cash / vault->issued_risk_units;
    return SG_OK;
}

sg_status_t sg_vault_preview_redeem(
    const sg_vault_t *vault,
    sg_amount_t risk_units,
    sg_amount_t *out,
    sg_error_t *err) {
    sg_amount_t gross = 0;
    sg_amount_t haircut = 0;
    sg_status_t status = SG_OK;
    if (vault == NULL || out == NULL) {
        sg_error_set(err, SG_ERR_INVALID_ARGUMENT, "vault and output are required");
        return SG_ERR_INVALID_ARGUMENT;
    }
    if (risk_units == 0) {
        sg_error_set(err, SG_ERR_INVALID_ARGUMENT, "risk units must be non-zero");
        return SG_ERR_INVALID_ARGUMENT;
    }
    if (risk_units > vault->issued_risk_units) {
        sg_error_set(err, SG_ERR_INSUFFICIENT_BALANCE, "risk units exceed issued supply");
        return SG_ERR_INSUFFICIENT_BALANCE;
    }
    status = sg_amount_mul_ratio_status(
        risk_units,
        vault->reserve_cash,
        vault->issued_risk_units,
        &gross,
        err);
    if (status != SG_OK) {
        return status;
    }
    if (vault->haircut_bps > 0) {
        if (!sg_amount_mul_bps(gross, vault->haircut_bps, &haircut)) {
            sg_error_set(err, SG_ERR_OVERFLOW, "haircut calculation overflow");
            return SG_ERR_OVERFLOW;
        }
    }
    if (gross < haircut) {
        sg_error_set(err, SG_ERR_INTERNAL, "haircut exceeds redemption amount");
        return SG_ERR_INTERNAL;
    }
    *out = gross - haircut;
    return SG_OK;
}

sg_status_t sg_vault_redeem(
    sg_vault_t *vault,
    sg_amount_t risk_units,
    sg_amount_t cash_out,
    sg_error_t *err) {
    sg_amount_t next_reserve = 0;
    sg_amount_t next_unit_value = 0;
    if (vault == NULL) {
        sg_error_set(err, SG_ERR_INVALID_ARGUMENT, "vault is required");
        return SG_ERR_INVALID_ARGUMENT;
    }
    if (risk_units == 0 || cash_out == 0) {
        sg_error_set(err, SG_ERR_INVALID_ARGUMENT, "redeem amounts must be non-zero");
        return SG_ERR_INVALID_ARGUMENT;
    }
    if (risk_units > vault->issued_risk_units) {
        sg_error_set(err, SG_ERR_INSUFFICIENT_BALANCE, "risk units exceed issued supply");
        return SG_ERR_INSUFFICIENT_BALANCE;
    }
    if (cash_out > sg_vault_liquid_cash(vault)) {
        sg_error_set(err, SG_ERR_INSUFFICIENT_BALANCE, "vault liquid cash is insufficient");
        return SG_ERR_INSUFFICIENT_BALANCE;
    }
    next_reserve = vault->reserve_cash - cash_out;
    if (next_reserve < vault->reserve_floor) {
        sg_error_set(err, SG_ERR_VAULT_STATE, "vault reserve floor would be crossed");
        return SG_ERR_VAULT_STATE;
    }
    vault->reserve_cash = next_reserve;
    vault->issued_risk_units -= risk_units;
    vault->version += 1;
    if (vault->issued_risk_units > 0 &&
        sg_vault_unit_value(vault, &next_unit_value, err) == SG_OK) {
        vault->last_unit_value = next_unit_value;
    }
    return SG_OK;
}

sg_status_t sg_vault_add_reserve(
    sg_vault_t *vault,
    sg_amount_t amount,
    sg_error_t *err) {
    sg_amount_t next = 0;
    sg_amount_t next_unit_value = 0;
    if (vault == NULL) {
        sg_error_set(err, SG_ERR_INVALID_ARGUMENT, "vault is required");
        return SG_ERR_INVALID_ARGUMENT;
    }
    if (amount == 0) {
        sg_error_set(err, SG_ERR_INVALID_ARGUMENT, "reserve amount must be non-zero");
        return SG_ERR_INVALID_ARGUMENT;
    }
    if (!sg_amount_add(vault->reserve_cash, amount, &next)) {
        sg_error_set(err, SG_ERR_OVERFLOW, "vault reserve overflow");
        return SG_ERR_OVERFLOW;
    }
    vault->reserve_cash = next;
    vault->version += 1;
    if (vault->issued_risk_units > 0 &&
        sg_vault_unit_value(vault, &next_unit_value, err) == SG_OK) {
        vault->last_unit_value = next_unit_value;
    }
    return SG_OK;
}

sg_status_t sg_vault_withdraw_reserve(
    sg_vault_t *vault,
    sg_amount_t amount,
    sg_error_t *err) {
    sg_amount_t next_unit_value = 0;
    if (vault == NULL) {
        sg_error_set(err, SG_ERR_INVALID_ARGUMENT, "vault is required");
        return SG_ERR_INVALID_ARGUMENT;
    }
    if (amount == 0) {
        sg_error_set(err, SG_ERR_INVALID_ARGUMENT, "reserve amount must be non-zero");
        return SG_ERR_INVALID_ARGUMENT;
    }
    if (amount > sg_vault_liquid_cash(vault)) {
        sg_error_set(err, SG_ERR_INSUFFICIENT_BALANCE, "vault liquid cash is insufficient");
        return SG_ERR_INSUFFICIENT_BALANCE;
    }
    if (vault->reserve_cash - amount < vault->reserve_floor) {
        sg_error_set(err, SG_ERR_VAULT_STATE, "vault reserve floor would be crossed");
        return SG_ERR_VAULT_STATE;
    }
    vault->reserve_cash -= amount;
    vault->version += 1;
    if (vault->issued_risk_units > 0 &&
        sg_vault_unit_value(vault, &next_unit_value, err) == SG_OK) {
        vault->last_unit_value = next_unit_value;
    }
    return SG_OK;
}

sg_status_t sg_vault_set_haircut(
    sg_vault_t *vault,
    uint32_t haircut_bps,
    sg_error_t *err) {
    if (vault == NULL) {
        sg_error_set(err, SG_ERR_INVALID_ARGUMENT, "vault is required");
        return SG_ERR_INVALID_ARGUMENT;
    }
    if (haircut_bps > SG_BPS_DENOMINATOR) {
        sg_error_set(err, SG_ERR_INVALID_ARGUMENT, "haircut exceeds denominator");
        return SG_ERR_INVALID_ARGUMENT;
    }
    if (vault->haircut_bps == haircut_bps) {
        return SG_OK;
    }
    vault->haircut_bps = haircut_bps;
    vault->version += 1;
    return SG_OK;
}

sg_status_t sg_vault_block_cash(
    sg_vault_t *vault,
    sg_amount_t amount,
    sg_error_t *err) {
    sg_amount_t liquid = 0;
    sg_amount_t next = 0;
    if (vault == NULL) {
        sg_error_set(err, SG_ERR_INVALID_ARGUMENT, "vault is required");
        return SG_ERR_INVALID_ARGUMENT;
    }
    liquid = sg_vault_liquid_cash(vault);
    if (amount == 0 || amount > liquid) {
        sg_error_set(err, SG_ERR_INSUFFICIENT_BALANCE, "vault liquid cash is insufficient");
        return SG_ERR_INSUFFICIENT_BALANCE;
    }
    if (!sg_amount_add(vault->blocked_cash, amount, &next)) {
        sg_error_set(err, SG_ERR_OVERFLOW, "blocked cash overflow");
        return SG_ERR_OVERFLOW;
    }
    vault->blocked_cash = next;
    vault->version += 1;
    return SG_OK;
}

sg_status_t sg_vault_release_blocked(
    sg_vault_t *vault,
    sg_amount_t amount,
    sg_error_t *err) {
    if (vault == NULL) {
        sg_error_set(err, SG_ERR_INVALID_ARGUMENT, "vault is required");
        return SG_ERR_INVALID_ARGUMENT;
    }
    if (amount == 0 || amount > vault->blocked_cash) {
        sg_error_set(err, SG_ERR_INSUFFICIENT_BALANCE, "blocked cash is insufficient");
        return SG_ERR_INSUFFICIENT_BALANCE;
    }
    vault->blocked_cash -= amount;
    vault->version += 1;
    return SG_OK;
}

bool sg_vault_reserve_floor_ok(const sg_vault_t *vault) {
    return vault != NULL && vault->reserve_cash >= vault->reserve_floor;
}

sg_amount_t sg_vault_liquid_cash(const sg_vault_t *vault) {
    if (vault == NULL || vault->reserve_cash < vault->blocked_cash) {
        return 0;
    }
    return vault->reserve_cash - vault->blocked_cash;
}
