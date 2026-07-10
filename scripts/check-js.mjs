import { readdirSync } from "node:fs";
import { join, resolve } from "node:path";
import { spawnSync } from "node:child_process";
import { fileURLToPath } from "node:url";

const root = resolve(fileURLToPath(new URL("..", import.meta.url)));
const targets = ["scripts", "tests"].flatMap((dir) => collect(join(root, dir)));

function collect(dir) {
  return readdirSync(dir, { withFileTypes: true }).flatMap((entry) => {
    const path = join(dir, entry.name);
    if (entry.isDirectory()) {
      return collect(path);
    }
    return entry.isFile() && (entry.name.endsWith(".js") || entry.name.endsWith(".mjs")) ? [path] : [];
  });
}

for (const file of targets) {
  const result = spawnSync(process.execPath, ["--check", file], { cwd: root, encoding: "utf8" });
  if (result.status !== 0) {
    process.stderr.write(result.stderr);
    process.exit(result.status ?? 1);
  }
}

console.log(`checked ${targets.length} JavaScript files`);
