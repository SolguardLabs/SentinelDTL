import assert from "node:assert/strict";
import { existsSync, mkdirSync, readdirSync } from "node:fs";
import { dirname, join, resolve } from "node:path";
import { fileURLToPath } from "node:url";
import { spawnSync } from "node:child_process";

const here = dirname(fileURLToPath(import.meta.url));
export const root = resolve(here, "..", "..");
const exeSuffix = process.platform === "win32" ? ".exe" : "";
const nativeTestBinary = join(root, "build", `sentineldtl-test-${process.pid}${exeSuffix}`);
const wslBinary = `./build/sentineldtl-test-${process.pid}`;
let cachedRunner = null;

function sourceFiles() {
  return readdirSync(join(root, "src"))
    .filter((file) => file.endsWith(".c"))
    .sort()
    .map((file) => join(root, "src", file));
}

function commandExists(command) {
  const probe =
    process.platform === "win32"
      ? spawnSync("where", [command], { stdio: "ignore" })
      : spawnSync("sh", ["-lc", `command -v ${command}`], { stdio: "ignore" });
  return probe.status === 0;
}

function chooseCompiler() {
  if (process.env.CC) {
    return process.env.CC;
  }
  const candidates = process.platform === "win32" ? ["clang", "gcc", "cc", "cl"] : ["cc", "gcc", "clang"];
  return candidates.find(commandExists) ?? null;
}

function shQuote(value) {
  return `'${value.replaceAll("'", "'\"'\"'")}'`;
}

function wslPath(path) {
  const result = spawnSync("wsl", ["wslpath", "-a", path], { encoding: "utf8" });
  if (result.status !== 0) {
    return null;
  }
  return result.stdout.trim();
}

function wslHasCompiler() {
  if (process.platform !== "win32" || !commandExists("wsl")) {
    return false;
  }
  const result = spawnSync("wsl", ["sh", "-lc", "command -v gcc || command -v clang || command -v cc"], {
    encoding: "utf8",
  });
  return result.status === 0 && result.stdout.trim().length > 0;
}

function compileWith(compiler, binary) {
  mkdirSync(dirname(binary), { recursive: true });
  const include = join(root, "include");
  const sources = sourceFiles();
  if (compiler.toLowerCase().endsWith("cl")) {
    return spawnSync(
      compiler,
      ["/nologo", "/W4", "/WX", "/std:c11", `/I${include}`, ...sources, `/Fe:${binary}`],
      { cwd: root, encoding: "utf8" },
    );
  }
  return spawnSync(
    compiler,
    ["-std=c11", "-Wall", "-Wextra", "-Werror", `-I${include}`, ...sources, "-o", binary],
    { cwd: root, encoding: "utf8" },
  );
}

function ensureRunner(t) {
  if (cachedRunner) {
    return cachedRunner;
  }
  if (process.env.SENTINELDTL_BIN) {
    cachedRunner = { type: "native", binary: process.env.SENTINELDTL_BIN };
    return cachedRunner;
  }
  if (existsSync(nativeTestBinary)) {
    cachedRunner = { type: "native", binary: nativeTestBinary };
    return cachedRunner;
  }
  const compiler = chooseCompiler();
  if (compiler) {
    const result = compileWith(compiler, nativeTestBinary);
    assert.equal(
      result.status,
      0,
      `C build failed with ${compiler}\nstdout:\n${result.stdout}\nstderr:\n${result.stderr}`,
    );
    cachedRunner = { type: "native", binary: nativeTestBinary };
    return cachedRunner;
  }
  if (wslHasCompiler()) {
    const mountedRoot = wslPath(root);
    if (!mountedRoot) {
      t.skip("WSL path conversion failed");
      return null;
    }
    const script =
      `cd ${shQuote(mountedRoot)} && ` +
      `mkdir -p build && ` +
      `SENTINELDTL_OUT=${shQuote(wslBinary)} bash scripts/build.sh`;
    const result = spawnSync("wsl", ["bash", "-lc", script], { cwd: root, encoding: "utf8" });
    assert.equal(
      result.status,
      0,
      `WSL C build failed\nstdout:\n${result.stdout}\nstderr:\n${result.stderr}`,
    );
    cachedRunner = { type: "wsl", root: mountedRoot, binary: wslBinary };
    return cachedRunner;
  }
  {
    t.skip("No C compiler is available in PATH");
    return null;
  }
}

export function ensureBinary(t) {
  const runner = ensureRunner(t);
  if (!runner) {
    return null;
  }
  return runner.type === "native" ? runner.binary : runner.binary;
}

export function runScenario(t, name) {
  const runner = ensureRunner(t);
  if (!runner) {
    return null;
  }
  const result =
    runner.type === "native"
      ? spawnSync(runner.binary, [name], { cwd: root, encoding: "utf8" })
      : spawnSync("wsl", ["sh", "-lc", `cd ${shQuote(runner.root)} && ${runner.binary} ${shQuote(name)}`], {
          cwd: root,
          encoding: "utf8",
        });
  assert.equal(
    result.status,
    0,
    `scenario ${name} failed\nstdout:\n${result.stdout}\nstderr:\n${result.stderr}`,
  );
  assert.equal(result.stderr, "");
  return JSON.parse(result.stdout);
}

export function eventKinds(doc) {
  return doc.events.map((event) => event.kind);
}

export function byId(items, id) {
  return items.find((item) => item.id === id);
}
