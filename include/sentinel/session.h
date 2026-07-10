#ifndef SENTINEL_SESSION_H
#define SENTINEL_SESSION_H

#include "sentinel/policy.h"

typedef struct sg_session {
    sg_id_t id;
    sg_id_t owner;
    sg_session_state_t state;
    sg_amount_t settled_notional;
    sg_amount_t queued_notional;
    sg_amount_t rejected_notional;
    sg_amount_t frozen_settled_notional;
    sg_amount_t frozen_queued_notional;
    sg_time_t opened_at;
    sg_time_t updated_at;
    uint32_t queued_orders;
    uint32_t executed_orders;
    uint32_t cancelled_orders;
    uint32_t rejected_orders;
    uint32_t pause_count;
    uint32_t resume_count;
    uint32_t protection_count;
} sg_session_t;

void sg_session_init(
    sg_session_t *session,
    const char *id,
    const char *owner,
    sg_time_t now);
bool sg_session_is_active(const sg_session_t *session);
bool sg_session_accepts_new_orders(const sg_session_t *session);
sg_amount_t sg_session_accounted_notional(const sg_session_t *session);
sg_amount_t sg_session_available_limit(
    const sg_session_t *session,
    const sg_policy_t *policy);
sg_status_t sg_session_reserve_order(
    sg_session_t *session,
    const sg_policy_t *policy,
    sg_amount_t admission_notional,
    sg_time_t now,
    sg_error_t *err);
sg_status_t sg_session_apply_execution(
    sg_session_t *session,
    const sg_policy_t *policy,
    sg_amount_t admission_notional,
    sg_amount_t execution_notional,
    sg_time_t now,
    sg_error_t *err);
sg_status_t sg_session_cancel_order(
    sg_session_t *session,
    sg_amount_t admission_notional,
    sg_time_t now,
    sg_error_t *err);
void sg_session_mark_rejection(
    sg_session_t *session,
    sg_amount_t notional,
    sg_time_t now);
void sg_session_pause(sg_session_t *session, sg_time_t now);
void sg_session_resume(sg_session_t *session, sg_time_t now);
void sg_session_enter_protection(sg_session_t *session, sg_time_t now);
void sg_session_close(sg_session_t *session, sg_time_t now);

#endif
