#ifndef SENTINEL_ORDER_H
#define SENTINEL_ORDER_H

#include "sentinel/vault.h"

typedef struct sg_order {
    sg_id_t id;
    sg_id_t session_id;
    sg_id_t owner;
    sg_order_kind_t kind;
    sg_order_status_t status;
    sg_amount_t risk_units;
    sg_amount_t admission_notional;
    sg_amount_t execution_notional;
    sg_amount_t min_cash_out;
    sg_amount_t fee_charged;
    sg_version_t admitted_vault_version;
    sg_version_t executed_vault_version;
    sg_time_t queued_at;
    sg_time_t executed_at;
    char note[SG_LABEL_LEN];
} sg_order_t;

void sg_order_clear(sg_order_t *order);
void sg_order_init_redeem(
    sg_order_t *order,
    const char *id,
    const char *session_id,
    const char *owner,
    sg_amount_t risk_units,
    sg_amount_t admission_notional,
    sg_amount_t min_cash_out,
    sg_version_t vault_version,
    sg_time_t now);
bool sg_order_is_live(const sg_order_t *order);
bool sg_order_belongs_to_session(const sg_order_t *order, const sg_id_t *session_id);
void sg_order_mark_executed(
    sg_order_t *order,
    sg_amount_t execution_notional,
    sg_amount_t fee_charged,
    sg_version_t vault_version,
    sg_time_t now);
void sg_order_mark_cancelled(sg_order_t *order, sg_time_t now);
void sg_order_mark_rejected(sg_order_t *order, sg_time_t now, const char *note);

#endif
