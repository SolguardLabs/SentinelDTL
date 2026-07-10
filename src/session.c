#include "sentinel/session.h"

void sg_session_init(
    sg_session_t *session,
    const char *id,
    const char *owner,
    sg_time_t now) {
    if (session == NULL) {
        return;
    }
    sg_id_from_text(&session->id, id);
    sg_id_from_text(&session->owner, owner);
    session->state = SG_SESSION_OPEN;
    session->settled_notional = 0;
    session->queued_notional = 0;
    session->rejected_notional = 0;
    session->frozen_settled_notional = 0;
    session->frozen_queued_notional = 0;
    session->opened_at = now;
    session->updated_at = now;
    session->queued_orders = 0;
    session->executed_orders = 0;
    session->cancelled_orders = 0;
    session->rejected_orders = 0;
    session->pause_count = 0;
    session->resume_count = 0;
    session->protection_count = 0;
}

bool sg_session_is_active(const sg_session_t *session) {
    return session != NULL && session->state == SG_SESSION_OPEN;
}

bool sg_session_accepts_new_orders(const sg_session_t *session) {
    return session != NULL && session->state == SG_SESSION_OPEN;
}

sg_amount_t sg_session_accounted_notional(const sg_session_t *session) {
    sg_amount_t total = 0;
    if (session == NULL) {
        return 0;
    }
    if (!sg_amount_add(session->settled_notional, session->queued_notional, &total)) {
        return UINT64_MAX;
    }
    return total;
}

sg_amount_t sg_session_available_limit(
    const sg_session_t *session,
    const sg_policy_t *policy) {
    sg_amount_t accounted = 0;
    if (session == NULL || policy == NULL) {
        return 0;
    }
    accounted = sg_session_accounted_notional(session);
    return sg_amount_saturating_sub(policy->session_limit, accounted);
}

sg_status_t sg_session_reserve_order(
    sg_session_t *session,
    const sg_policy_t *policy,
    sg_amount_t admission_notional,
    sg_time_t now,
    sg_error_t *err) {
    sg_amount_t after_queue = 0;
    sg_amount_t after_total = 0;
    sg_status_t status = SG_OK;
    if (session == NULL || policy == NULL) {
        sg_error_set(err, SG_ERR_INVALID_ARGUMENT, "session and policy are required");
        return SG_ERR_INVALID_ARGUMENT;
    }
    if (!sg_session_accepts_new_orders(session)) {
        sg_error_set(err, SG_ERR_PAUSED, "session is not accepting new orders");
        return SG_ERR_PAUSED;
    }
    status = sg_policy_check_order(
        policy,
        admission_notional,
        session->queued_notional,
        session->queued_orders,
        err);
    if (status != SG_OK) {
        return status;
    }
    if (!sg_amount_add(session->queued_notional, admission_notional, &after_queue)) {
        sg_error_set(err, SG_ERR_OVERFLOW, "queued notional overflow");
        return SG_ERR_OVERFLOW;
    }
    if (!sg_amount_add(session->settled_notional, after_queue, &after_total)) {
        sg_error_set(err, SG_ERR_OVERFLOW, "session notional overflow");
        return SG_ERR_OVERFLOW;
    }
    if (after_total > policy->session_limit) {
        sg_error_set(err, SG_ERR_LIMIT, "session limit reached");
        return SG_ERR_LIMIT;
    }
    session->queued_notional = after_queue;
    session->queued_orders += 1;
    session->updated_at = now;
    return SG_OK;
}

sg_status_t sg_session_apply_execution(
    sg_session_t *session,
    const sg_policy_t *policy,
    sg_amount_t admission_notional,
    sg_amount_t execution_notional,
    sg_time_t now,
    sg_error_t *err) {
    sg_amount_t remaining_queue = 0;
    sg_amount_t booked_notional = execution_notional;
    sg_amount_t next_settled = 0;
    sg_amount_t next_total = 0;
    if (session == NULL || policy == NULL) {
        sg_error_set(err, SG_ERR_INVALID_ARGUMENT, "session and policy are required");
        return SG_ERR_INVALID_ARGUMENT;
    }
    if (session->state == SG_SESSION_CLOSED) {
        sg_error_set(err, SG_ERR_ORDER_STATE, "session is closed");
        return SG_ERR_ORDER_STATE;
    }
    if (session->queued_notional < admission_notional) {
        sg_error_set(err, SG_ERR_INTERNAL, "queued notional is inconsistent");
        return SG_ERR_INTERNAL;
    }
    if (!policy->execute_during_protection && session->state == SG_SESSION_PROTECTION) {
        sg_error_set(err, SG_ERR_PROTECTION, "execution is not enabled in protection");
        return SG_ERR_PROTECTION;
    }
    if (session->state == SG_SESSION_PROTECTION) {
        booked_notional = admission_notional;
    }
    remaining_queue = session->queued_notional - admission_notional;
    if (!sg_amount_add(session->settled_notional, booked_notional, &next_settled)) {
        sg_error_set(err, SG_ERR_OVERFLOW, "settled notional overflow");
        return SG_ERR_OVERFLOW;
    }
    if (!sg_amount_add(next_settled, remaining_queue, &next_total)) {
        sg_error_set(err, SG_ERR_OVERFLOW, "session notional overflow");
        return SG_ERR_OVERFLOW;
    }
    if (next_total > policy->session_limit) {
        sg_error_set(err, SG_ERR_LIMIT, "session limit reached during execution");
        return SG_ERR_LIMIT;
    }
    session->queued_notional = remaining_queue;
    session->settled_notional = next_settled;
    if (session->queued_orders > 0) {
        session->queued_orders -= 1;
    }
    session->executed_orders += 1;
    session->updated_at = now;
    return SG_OK;
}

sg_status_t sg_session_cancel_order(
    sg_session_t *session,
    sg_amount_t admission_notional,
    sg_time_t now,
    sg_error_t *err) {
    if (session == NULL) {
        sg_error_set(err, SG_ERR_INVALID_ARGUMENT, "session is required");
        return SG_ERR_INVALID_ARGUMENT;
    }
    if (session->queued_notional < admission_notional) {
        sg_error_set(err, SG_ERR_INTERNAL, "queued notional is inconsistent");
        return SG_ERR_INTERNAL;
    }
    session->queued_notional -= admission_notional;
    if (session->queued_orders > 0) {
        session->queued_orders -= 1;
    }
    session->cancelled_orders += 1;
    session->updated_at = now;
    return SG_OK;
}

void sg_session_mark_rejection(
    sg_session_t *session,
    sg_amount_t notional,
    sg_time_t now) {
    sg_amount_t next_rejected = 0;
    if (session == NULL) {
        return;
    }
    if (sg_amount_add(session->rejected_notional, notional, &next_rejected)) {
        session->rejected_notional = next_rejected;
    }
    session->rejected_orders += 1;
    session->updated_at = now;
}

void sg_session_pause(sg_session_t *session, sg_time_t now) {
    if (session == NULL || session->state == SG_SESSION_CLOSED) {
        return;
    }
    session->state = SG_SESSION_PAUSED;
    session->pause_count += 1;
    session->updated_at = now;
}

void sg_session_resume(sg_session_t *session, sg_time_t now) {
    if (session == NULL || session->state == SG_SESSION_CLOSED) {
        return;
    }
    session->state = SG_SESSION_OPEN;
    session->resume_count += 1;
    session->updated_at = now;
}

void sg_session_enter_protection(sg_session_t *session, sg_time_t now) {
    if (session == NULL || session->state == SG_SESSION_CLOSED) {
        return;
    }
    session->frozen_settled_notional = session->settled_notional;
    session->frozen_queued_notional = session->queued_notional;
    session->state = SG_SESSION_PROTECTION;
    session->protection_count += 1;
    session->updated_at = now;
}

void sg_session_close(sg_session_t *session, sg_time_t now) {
    if (session == NULL) {
        return;
    }
    session->state = SG_SESSION_CLOSED;
    session->updated_at = now;
}
