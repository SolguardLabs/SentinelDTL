import assert from "node:assert/strict";
import test from "node:test";
import { byId, eventKinds, runScenario } from "../helpers/run.js";

test("baseline scenario settles a queued redemption", (t) => {
  const doc = runScenario(t, "baseline");
  if (!doc) return;

  assert.equal(doc.scenario, "baseline");
  assert.equal(doc.mode, "active");
  assert.equal(doc.checks.cash_conservation, true);
  assert.equal(doc.checks.unit_conservation, true);
  assert.equal(doc.vault.floor_ok, true);

  const alice = byId(doc.accounts, "alice");
  const session = byId(doc.sessions, "alice-day");
  const [order] = doc.orders;

  assert.equal(alice.cash_balance, 49920);
  assert.equal(alice.risk_units, 4600);
  assert.equal(alice.queued_units, 0);
  assert.equal(session.settled_notional, 40000);
  assert.equal(session.queued_notional, 0);
  assert.equal(session.executed_orders, 1);
  assert.equal(order.status, "executed");
  assert.equal(order.admission_notional, 40000);
  assert.equal(order.execution_notional, 40000);
  assert.deepEqual(eventKinds(doc).slice(0, 5), [
    "boot",
    "account_created",
    "account_created",
    "account_created",
    "session_opened",
  ]);
});

test("snapshot scenario exposes deterministic startup state", (t) => {
  const doc = runScenario(t, "snapshot");
  if (!doc) return;

  assert.equal(doc.scenario, "snapshot");
  assert.equal(doc.orders.length, 0);
  assert.equal(doc.vault.reserve_cash, 900000);
  assert.equal(doc.vault.issued_risk_units, 9000);
  assert.equal(doc.vault.unit_value, 100);
  assert.equal(doc.checks.account_unit_total, 9000);
  assert.equal(doc.checks.cash_conservation, true);
});
