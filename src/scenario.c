#include "sentinel/scenario.h"

#include <stdio.h>
#include <string.h>

static sg_status_t sg_require(sg_status_t status, const sg_error_t *source, sg_error_t *err) {
    if (status == SG_OK) {
        return SG_OK;
    }
    if (source != NULL) {
        sg_error_set(err, source->status, source->message);
    }
    return status;
}

static sg_status_t sg_seed_engine(sg_engine_t *engine, sg_error_t *err) {
    sg_error_t local;
    sg_status_t status = SG_OK;
    sg_error_clear(&local);
    status = sg_engine_bootstrap(engine, &local);
    if (status != SG_OK) return sg_require(status, &local, err);
    status = sg_engine_add_account(engine, "alice", 10000, 5000, &local);
    if (status != SG_OK) return sg_require(status, &local, err);
    status = sg_engine_add_account(engine, "bob", 15000, 2500, &local);
    if (status != SG_OK) return sg_require(status, &local, err);
    status = sg_engine_add_account(engine, "maker", 25000, 1500, &local);
    if (status != SG_OK) return sg_require(status, &local, err);
    return SG_OK;
}

static sg_status_t sg_emit(
    const char *scenario,
    const sg_engine_t *engine,
    const sg_error_t *last_error,
    FILE *out,
    sg_error_t *err) {
    sg_json_t json;
    sg_status_t status = SG_OK;
    sg_json_init(&json);
    status = sg_json_write_engine(&json, engine, scenario, last_error, err);
    if (status == SG_OK) {
        fprintf(out, "%s\n", json.data == NULL ? "{}" : json.data);
    }
    sg_json_free(&json);
    return status;
}

static sg_status_t sg_scenario_baseline(FILE *out, sg_error_t *err) {
    sg_engine_t engine;
    sg_error_t local;
    char order_id[SG_ID_LEN];
    sg_status_t status = SG_OK;
    sg_error_clear(&local);
    status = sg_seed_engine(&engine, err);
    if (status != SG_OK) return status;
    status = sg_engine_open_session(&engine, "alice-day", "alice", &local);
    if (status != SG_OK) return sg_require(status, &local, err);
    status = sg_engine_enqueue_redeem(
        &engine,
        "alice-day",
        "alice",
        400,
        39000,
        order_id,
        sizeof(order_id),
        &local);
    if (status != SG_OK) return sg_require(status, &local, err);
    status = sg_engine_execute_order(&engine, order_id, &local);
    if (status != SG_OK) return sg_require(status, &local, err);
    sg_error_clear(&local);
    return sg_emit("baseline", &engine, &local, out, err);
}

static sg_status_t sg_scenario_pause_resume(FILE *out, sg_error_t *err) {
    sg_engine_t engine;
    sg_error_t local;
    sg_error_t observed;
    char first_order[SG_ID_LEN];
    char second_order[SG_ID_LEN];
    sg_status_t status = SG_OK;
    sg_error_clear(&local);
    sg_error_clear(&observed);
    status = sg_seed_engine(&engine, err);
    if (status != SG_OK) return status;
    status = sg_engine_open_session(&engine, "alice-ops", "alice", &local);
    if (status != SG_OK) return sg_require(status, &local, err);
    status = sg_engine_enqueue_redeem(
        &engine,
        "alice-ops",
        "alice",
        300,
        29000,
        first_order,
        sizeof(first_order),
        &local);
    if (status != SG_OK) return sg_require(status, &local, err);
    status = sg_engine_pause(&engine, "operator pause", &local);
    if (status != SG_OK) return sg_require(status, &local, err);
    status = sg_engine_enqueue_redeem(
        &engine,
        "alice-ops",
        "alice",
        100,
        9500,
        second_order,
        sizeof(second_order),
        &observed);
    if (status == SG_OK) {
        sg_error_set(err, SG_ERR_INTERNAL, "paused enqueue unexpectedly succeeded");
        return SG_ERR_INTERNAL;
    }
    status = sg_engine_resume(&engine, "operator resume", &local);
    if (status != SG_OK) return sg_require(status, &local, err);
    status = sg_engine_enqueue_redeem(
        &engine,
        "alice-ops",
        "alice",
        200,
        19000,
        second_order,
        sizeof(second_order),
        &local);
    if (status != SG_OK) return sg_require(status, &local, err);
    status = sg_engine_execute_order(&engine, first_order, &local);
    if (status != SG_OK) return sg_require(status, &local, err);
    status = sg_engine_execute_order(&engine, second_order, &local);
    if (status != SG_OK) return sg_require(status, &local, err);
    return sg_emit("pause_resume", &engine, &observed, out, err);
}

static sg_status_t sg_scenario_limits(FILE *out, sg_error_t *err) {
    sg_engine_t engine;
    sg_error_t local;
    sg_error_t observed;
    char order_id[SG_ID_LEN];
    sg_status_t status = SG_OK;
    sg_error_clear(&local);
    sg_error_clear(&observed);
    status = sg_seed_engine(&engine, err);
    if (status != SG_OK) return status;
    engine.policy.session_limit = 80000;
    engine.policy.queue_limit = 80000;
    engine.policy.single_order_limit = 75000;
    status = sg_engine_open_session(&engine, "alice-limit", "alice", &local);
    if (status != SG_OK) return sg_require(status, &local, err);
    status = sg_engine_enqueue_redeem(
        &engine,
        "alice-limit",
        "alice",
        700,
        69000,
        order_id,
        sizeof(order_id),
        &local);
    if (status != SG_OK) return sg_require(status, &local, err);
    status = sg_engine_enqueue_redeem(
        &engine,
        "alice-limit",
        "alice",
        200,
        19000,
        order_id,
        sizeof(order_id),
        &observed);
    if (status == SG_OK) {
        sg_error_set(err, SG_ERR_INTERNAL, "limit enqueue unexpectedly succeeded");
        return SG_ERR_INTERNAL;
    }
    return sg_emit("limits", &engine, &observed, out, err);
}

static sg_status_t sg_scenario_protection(FILE *out, sg_error_t *err) {
    sg_engine_t engine;
    sg_error_t local;
    sg_error_t observed;
    char order_id[SG_ID_LEN];
    char rejected_id[SG_ID_LEN];
    sg_status_t status = SG_OK;
    sg_error_clear(&local);
    sg_error_clear(&observed);
    status = sg_seed_engine(&engine, err);
    if (status != SG_OK) return status;
    status = sg_engine_open_session(&engine, "alice-protect", "alice", &local);
    if (status != SG_OK) return sg_require(status, &local, err);
    status = sg_engine_enqueue_redeem(
        &engine,
        "alice-protect",
        "alice",
        700,
        69000,
        order_id,
        sizeof(order_id),
        &local);
    if (status != SG_OK) return sg_require(status, &local, err);
    status = sg_engine_enter_protection(&engine, "risk guard", &local);
    if (status != SG_OK) return sg_require(status, &local, err);
    status = sg_engine_add_vault_reserve(&engine, 540000, "reserve rebalance", &local);
    if (status != SG_OK) return sg_require(status, &local, err);
    status = sg_engine_enqueue_redeem(
        &engine,
        "alice-protect",
        "alice",
        100,
        9000,
        rejected_id,
        sizeof(rejected_id),
        &observed);
    if (status == SG_OK) {
        sg_error_set(err, SG_ERR_INTERNAL, "protection enqueue unexpectedly succeeded");
        return SG_ERR_INTERNAL;
    }
    status = sg_engine_execute_order(&engine, order_id, &local);
    if (status != SG_OK) return sg_require(status, &local, err);
    return sg_emit("protection", &engine, &observed, out, err);
}

static sg_status_t sg_scenario_batch(FILE *out, sg_error_t *err) {
    sg_engine_t engine;
    sg_error_t local;
    char alice_first[SG_ID_LEN];
    char alice_second[SG_ID_LEN];
    char bob_first[SG_ID_LEN];
    sg_status_t status = SG_OK;
    sg_error_clear(&local);
    status = sg_seed_engine(&engine, err);
    if (status != SG_OK) return status;
    status = sg_engine_open_session(&engine, "alice-batch", "alice", &local);
    if (status != SG_OK) return sg_require(status, &local, err);
    status = sg_engine_open_session(&engine, "bob-batch", "bob", &local);
    if (status != SG_OK) return sg_require(status, &local, err);
    status = sg_engine_enqueue_redeem(&engine, "alice-batch", "alice", 250, 24000, alice_first, sizeof(alice_first), &local);
    if (status != SG_OK) return sg_require(status, &local, err);
    status = sg_engine_enqueue_redeem(&engine, "alice-batch", "alice", 350, 34000, alice_second, sizeof(alice_second), &local);
    if (status != SG_OK) return sg_require(status, &local, err);
    status = sg_engine_enqueue_redeem(&engine, "bob-batch", "bob", 300, 29000, bob_first, sizeof(bob_first), &local);
    if (status != SG_OK) return sg_require(status, &local, err);
    status = sg_engine_execute_next(&engine, "alice-batch", &local);
    if (status != SG_OK) return sg_require(status, &local, err);
    status = sg_engine_execute_next(&engine, "bob-batch", &local);
    if (status != SG_OK) return sg_require(status, &local, err);
    status = sg_engine_cancel_order(&engine, alice_second, &local);
    if (status != SG_OK) return sg_require(status, &local, err);
    sg_error_clear(&local);
    return sg_emit("batch", &engine, &local, out, err);
}

static sg_status_t sg_scenario_treasury(FILE *out, sg_error_t *err) {
    sg_engine_t engine;
    sg_error_t local;
    char order_id[SG_ID_LEN];
    sg_status_t status = SG_OK;
    sg_error_clear(&local);
    status = sg_seed_engine(&engine, err);
    if (status != SG_OK) return status;
    status = sg_engine_transfer_cash(&engine, "maker", "bob", 2000, "operator settlement", &local);
    if (status != SG_OK) return sg_require(status, &local, err);
    status = sg_engine_contribute_to_vault(&engine, "maker", 10000, "reserve contribution", &local);
    if (status != SG_OK) return sg_require(status, &local, err);
    status = sg_engine_withdraw_vault_buffer(&engine, "maker", 5000, "buffer normalization", &local);
    if (status != SG_OK) return sg_require(status, &local, err);
    status = sg_engine_set_vault_haircut(&engine, 100, "daily haircut", &local);
    if (status != SG_OK) return sg_require(status, &local, err);
    status = sg_engine_open_session(&engine, "bob-treasury", "bob", &local);
    if (status != SG_OK) return sg_require(status, &local, err);
    status = sg_engine_enqueue_redeem(
        &engine,
        "bob-treasury",
        "bob",
        100,
        9800,
        order_id,
        sizeof(order_id),
        &local);
    if (status != SG_OK) return sg_require(status, &local, err);
    status = sg_engine_execute_order(&engine, order_id, &local);
    if (status != SG_OK) return sg_require(status, &local, err);
    sg_error_clear(&local);
    return sg_emit("treasury", &engine, &local, out, err);
}

static sg_status_t sg_scenario_snapshot(FILE *out, sg_error_t *err) {
    sg_engine_t engine;
    sg_error_t local;
    sg_status_t status = SG_OK;
    sg_error_clear(&local);
    status = sg_seed_engine(&engine, err);
    if (status != SG_OK) return status;
    status = sg_engine_open_session(&engine, "alice-snapshot", "alice", &local);
    if (status != SG_OK) return sg_require(status, &local, err);
    status = sg_event_push(
        &engine.events,
        SG_EVENT_CHECKPOINT,
        engine.clock,
        "control-plane",
        engine.vault.reserve_cash,
        engine.vault.issued_risk_units,
        engine.vault.version,
        "checkpoint",
        &local);
    if (status != SG_OK) return sg_require(status, &local, err);
    sg_error_clear(&local);
    return sg_emit("snapshot", &engine, &local, out, err);
}

sg_status_t sg_scenario_run(const char *name, FILE *out, sg_error_t *err) {
    if (name == NULL || name[0] == '\0') {
        name = "baseline";
    }
    if (out == NULL) {
        sg_error_set(err, SG_ERR_INVALID_ARGUMENT, "output stream is required");
        return SG_ERR_INVALID_ARGUMENT;
    }
    if (strcmp(name, "baseline") == 0) {
        return sg_scenario_baseline(out, err);
    }
    if (strcmp(name, "pause-resume") == 0 || strcmp(name, "pause_resume") == 0) {
        return sg_scenario_pause_resume(out, err);
    }
    if (strcmp(name, "limits") == 0) {
        return sg_scenario_limits(out, err);
    }
    if (strcmp(name, "protection") == 0) {
        return sg_scenario_protection(out, err);
    }
    if (strcmp(name, "batch") == 0) {
        return sg_scenario_batch(out, err);
    }
    if (strcmp(name, "treasury") == 0) {
        return sg_scenario_treasury(out, err);
    }
    if (strcmp(name, "snapshot") == 0) {
        return sg_scenario_snapshot(out, err);
    }
    sg_error_set(err, SG_ERR_INVALID_ARGUMENT, "unknown scenario");
    return SG_ERR_INVALID_ARGUMENT;
}

void sg_scenario_print_list(FILE *out) {
    if (out == NULL) {
        return;
    }
    fprintf(out, "baseline\n");
    fprintf(out, "pause-resume\n");
    fprintf(out, "limits\n");
    fprintf(out, "protection\n");
    fprintf(out, "batch\n");
    fprintf(out, "treasury\n");
    fprintf(out, "snapshot\n");
}
