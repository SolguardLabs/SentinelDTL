#include "sentinel/order.h"

#include <string.h>

static void sg_order_copy_note(char *dst, size_t cap, const char *src) {
    if (cap == 0) {
        return;
    }
    if (src == NULL) {
        dst[0] = '\0';
        return;
    }
    strncpy(dst, src, cap - 1);
    dst[cap - 1] = '\0';
}

void sg_order_clear(sg_order_t *order) {
    if (order == NULL) {
        return;
    }
    memset(order, 0, sizeof(*order));
    order->kind = SG_ORDER_REDEEM_RISK;
    order->status = SG_ORDER_EMPTY;
}

void sg_order_init_redeem(
    sg_order_t *order,
    const char *id,
    const char *session_id,
    const char *owner,
    sg_amount_t risk_units,
    sg_amount_t admission_notional,
    sg_amount_t min_cash_out,
    sg_version_t vault_version,
    sg_time_t now) {
    if (order == NULL) {
        return;
    }
    sg_order_clear(order);
    sg_id_from_text(&order->id, id);
    sg_id_from_text(&order->session_id, session_id);
    sg_id_from_text(&order->owner, owner);
    order->kind = SG_ORDER_REDEEM_RISK;
    order->status = SG_ORDER_QUEUED;
    order->risk_units = risk_units;
    order->admission_notional = admission_notional;
    order->execution_notional = 0;
    order->min_cash_out = min_cash_out;
    order->fee_charged = 0;
    order->admitted_vault_version = vault_version;
    order->executed_vault_version = 0;
    order->queued_at = now;
    order->executed_at = 0;
    sg_order_copy_note(order->note, sizeof(order->note), "queued");
}

bool sg_order_is_live(const sg_order_t *order) {
    return order != NULL && order->status == SG_ORDER_QUEUED;
}

bool sg_order_belongs_to_session(const sg_order_t *order, const sg_id_t *session_id) {
    if (order == NULL || session_id == NULL) {
        return false;
    }
    return sg_id_eq(&order->session_id, session_id);
}

void sg_order_mark_executed(
    sg_order_t *order,
    sg_amount_t execution_notional,
    sg_amount_t fee_charged,
    sg_version_t vault_version,
    sg_time_t now) {
    if (order == NULL) {
        return;
    }
    order->status = SG_ORDER_EXECUTED;
    order->execution_notional = execution_notional;
    order->fee_charged = fee_charged;
    order->executed_vault_version = vault_version;
    order->executed_at = now;
    sg_order_copy_note(order->note, sizeof(order->note), "executed");
}

void sg_order_mark_cancelled(sg_order_t *order, sg_time_t now) {
    if (order == NULL) {
        return;
    }
    order->status = SG_ORDER_CANCELLED;
    order->executed_at = now;
    sg_order_copy_note(order->note, sizeof(order->note), "cancelled");
}

void sg_order_mark_rejected(sg_order_t *order, sg_time_t now, const char *note) {
    if (order == NULL) {
        return;
    }
    order->status = SG_ORDER_REJECTED;
    order->executed_at = now;
    sg_order_copy_note(order->note, sizeof(order->note), note == NULL ? "rejected" : note);
}
