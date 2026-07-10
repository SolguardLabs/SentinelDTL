import assert from "node:assert/strict";
import test from "node:test";
import { byId, eventKinds, runScenario } from "../helpers/run.js";

test("batch scenario supports multi-session execution and cancellation", (t) => {
  const doc = runScenario(t, "batch");
  if (!doc) return;

  const alice = byId(doc.accounts, "alice");
  const bob = byId(doc.accounts, "bob");
  const aliceSession = byId(doc.sessions, "alice-batch");
  const bobSession = byId(doc.sessions, "bob-batch");
  const statuses = doc.orders.map((order) => order.status).sort();

  assert.deepEqual(statuses, ["cancelled", "executed", "executed"]);
  assert.equal(alice.queued_units, 0);
  assert.equal(bob.queued_units, 0);
  assert.equal(aliceSession.executed_orders, 1);
  assert.equal(aliceSession.cancelled_orders, 1);
  assert.equal(bobSession.executed_orders, 1);
  assert.equal(eventKinds(doc).filter((kind) => kind === "order_executed").length, 2);
  assert.equal(eventKinds(doc).filter((kind) => kind === "order_cancelled").length, 1);
  assert.equal(doc.checks.cash_conservation, true);
  assert.equal(doc.checks.unit_conservation, true);
});
