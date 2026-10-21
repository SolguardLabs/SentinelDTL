#include "sentinel/analytics.h"
#include "sentinel/error.h"

#include <assert.h>
#include <stdio.h>

static void test_base_capacity(void) {
    sg_vault_t vault;
    sg_liquidity_envelope_t view;
    sg_error_t err;
    sg_error_clear(&err);
    sg_vault_init(&vault, "main-vault", 900000, 9000, 250000);
    vault.blocked_cash = 100000;
    assert(sg_liquidity_envelope_from_vault(&vault, 1000, &view, &err) == SG_OK);
    assert(view.liquid_cash == 800000);
    assert(view.reserve_headroom == 650000);
    assert(view.redeem_capacity == 650000);
    assert(view.stress_loss == 90000);
    assert(view.stressed_reserve == 810000);
    assert(view.stressed_headroom == 560000);
    assert(view.stressed_redeem_capacity == 560000);
    assert(view.floor_covered_after_stress);
}

static void test_floor_and_invalid_input(void) {
    sg_vault_t vault;
    sg_liquidity_envelope_t view;
    sg_error_t err;
    sg_error_clear(&err);
    sg_vault_init(&vault, "main-vault", 300000, 3000, 250000);
    assert(sg_liquidity_envelope_from_vault(&vault, 2000, &view, &err) == SG_OK);
    assert(view.stressed_reserve == 240000);
    assert(view.stressed_redeem_capacity == 0);
    assert(!view.floor_covered_after_stress);
    assert(sg_liquidity_envelope_from_vault(&vault, 10001, &view, &err) == SG_ERR_INVALID_ARGUMENT);
    vault.blocked_cash = 300001;
    assert(sg_liquidity_envelope_from_vault(&vault, 1000, &view, &err) == SG_ERR_VAULT_STATE);
}

int main(void) {
    test_base_capacity();
    test_floor_and_invalid_input();
    puts("analytics: 2 passed");
    return 0;
}
