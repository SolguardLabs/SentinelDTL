import assert from "node:assert/strict";
import test from "node:test";
import { byId, eventKinds, runScenario } from "../helpers/run.js";

test("risk snapshot reports queued exposure and utilization bands", (t) => {
  const doc = runScenario(t, "limits");
  if (!doc) return;

  assert.equal(doc.risk.live_orders, 1);
  assert.equal(doc.risk.live_order_notional, 70000);
  assert.equal(doc.risk.live_order_units, 700);
  assert.equal(doc.risk.queued_risk_units, 700);
  assert.equal(doc.risk.queue_utilization_bps, 8750);
  assert.equal(doc.risk.session_utilization_bps, 8750);
  assert.equal(doc.risk.session_risk_band, "elevated");
  assert.equal(doc.risk.balanced, true);
});

test("treasury scenario covers transfers, vault buffers, haircut and risk metrics", (t) => {
  const doc = runScenario(t, "treasury");
  if (!doc) return;

  const bob = byId(doc.accounts, "bob");
  const maker = byId(doc.accounts, "maker");
  const session = byId(doc.sessions, "bob-treasury");
  const [order] = doc.orders;
  const kinds = eventKinds(doc);

  assert.equal(doc.scenario, "treasury");
  assert.equal(doc.mode, "active");
  assert.equal(doc.last_error.status, "ok");
  assert.equal(doc.vault.reserve_cash, 895064);
  assert.equal(doc.vault.haircut_bps, 100);
  assert.equal(doc.vault.version, 5);
  assert.equal(bob.cash_balance, 26936);
  assert.equal(bob.credited_cash, 11936);
  assert.equal(maker.cash_balance, 18000);
  assert.equal(maker.debited_cash, 12000);
  assert.equal(maker.credited_cash, 5000);
  assert.equal(session.settled_notional, 9955);
  assert.equal(session.executed_orders, 1);
  assert.equal(order.admission_notional, 9955);
  assert.equal(order.execution_notional, 9955);
  assert.equal(order.fee_charged, 19);
  assert.equal(doc.risk.account_cash, 54936);
  assert.equal(doc.risk.vault_cash, 895064);
  assert.equal(doc.risk.total_cash, 950000);
  assert.equal(doc.risk.live_orders, 0);
  assert.equal(doc.risk.balanced, true);
  assert.equal(kinds.filter((kind) => kind === "vault_adjusted").length, 2);
  assert.equal(kinds.filter((kind) => kind === "checkpoint").length, 2);
  assert.equal(doc.checks.cash_conservation, true);
  assert.equal(doc.checks.unit_conservation, true);
});
