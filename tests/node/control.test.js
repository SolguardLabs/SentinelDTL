import assert from "node:assert/strict";
import test from "node:test";
import { byId, eventKinds, runScenario } from "../helpers/run.js";

test("pause and resume preserve queued work and block fresh intake", (t) => {
  const doc = runScenario(t, "pause-resume");
  if (!doc) return;

  const session = byId(doc.sessions, "alice-ops");
  const alice = byId(doc.accounts, "alice");

  assert.equal(doc.scenario, "pause_resume");
  assert.equal(doc.mode, "active");
  assert.equal(doc.last_error.status, "paused");
  assert.equal(session.state, "open");
  assert.equal(session.pause_count, 1);
  assert.equal(session.resume_count, 1);
  assert.equal(session.executed_orders, 2);
  assert.equal(session.queued_orders, 0);
  assert.equal(alice.queued_units, 0);
  assert.equal(doc.orders.every((order) => order.status === "executed"), true);
  assert.equal(eventKinds(doc).includes("paused"), true);
  assert.equal(eventKinds(doc).includes("resumed"), true);
  assert.equal(doc.checks.cash_conservation, true);
});

test("session limits retain queued accounting after a rejected intake", (t) => {
  const doc = runScenario(t, "limits");
  if (!doc) return;

  const session = byId(doc.sessions, "alice-limit");

  assert.equal(doc.mode, "active");
  assert.equal(doc.last_error.status, "limit");
  assert.equal(doc.orders.length, 1);
  assert.equal(doc.orders[0].status, "queued");
  assert.equal(session.queued_notional, 70000);
  assert.equal(session.rejected_notional, 20000);
  assert.equal(session.queued_orders, 1);
  assert.equal(session.rejected_orders, 1);
  assert.equal(doc.checks.cash_conservation, true);
  assert.equal(doc.checks.unit_conservation, true);
});

test("protection mode blocks new intake while processing admitted queue", (t) => {
  const doc = runScenario(t, "protection");
  if (!doc) return;

  const session = byId(doc.sessions, "alice-protect");
  const alice = byId(doc.accounts, "alice");

  assert.equal(doc.mode, "protection");
  assert.equal(doc.last_error.status, "protection");
  assert.equal(session.state, "protection");
  assert.equal(session.protection_count, 1);
  assert.equal(session.frozen_queued_notional, 70000);
  assert.equal(session.queued_orders, 0);
  assert.equal(session.executed_orders, 1);
  assert.equal(alice.queued_units, 0);
  assert.equal(doc.orders[0].status, "executed");
  assert.equal(doc.vault.floor_ok, true);
  assert.equal(doc.checks.cash_conservation, true);
  assert.equal(doc.checks.unit_conservation, true);
});
