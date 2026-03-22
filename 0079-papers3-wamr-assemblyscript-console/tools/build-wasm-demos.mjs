#!/usr/bin/env node

import { execFileSync } from "node:child_process";
import { copyFileSync, existsSync, mkdirSync, rmSync, writeFileSync } from "node:fs";
import { dirname, join, resolve } from "node:path";
import { fileURLToPath } from "node:url";

const __filename = fileURLToPath(import.meta.url);
const __dirname = dirname(__filename);
const projectRoot = resolve(__dirname, "..");
const wasmSrcRoot = join(projectRoot, "wasm-src");
const outputRoot = join(projectRoot, "wasm-build");
const embeddedAssetRoot = join(projectRoot, "main", "wasm-assets");
const target = getFlagValue("--target") ?? "release";

const demos = [
  "hello-frame",
  "nested-boxes",
  "bars",
  "checkerboard",
  "radar-sweep"
];

const ascBinary = process.platform === "win32"
  ? join(wasmSrcRoot, "node_modules", ".bin", "asc.cmd")
  : join(wasmSrcRoot, "node_modules", ".bin", "asc");

if (!existsSync(ascBinary)) {
  console.error(`AssemblyScript compiler not found at ${ascBinary}`);
  console.error("Run `npm install` inside `wasm-src/` first.");
  process.exit(1);
}

const targetOutputRoot = join(outputRoot, target);
rmSync(targetOutputRoot, { recursive: true, force: true });
mkdirSync(targetOutputRoot, { recursive: true });

for (const demo of demos) {
  const sourceFile = `${demo}/assembly/index.ts`;
  const wasmFile = join(targetOutputRoot, `${demo}.wasm`);
  const watFile = join(targetOutputRoot, `${demo}.wat`);

  console.log(`Building ${demo} (${target})`);
  execFileSync(
    ascBinary,
    [
      sourceFile,
      "--config", "asconfig.json",
      "--target", target,
      "--outFile", wasmFile,
      "--textFile", watFile
    ],
    {
      cwd: wasmSrcRoot,
      stdio: "inherit"
    }
  );
}

writeFileSync(
  join(targetOutputRoot, "manifest.json"),
  JSON.stringify(
    {
      target,
      demos: demos.map((name) => ({
        name,
        wasm: `${name}.wasm`,
        wat: `${name}.wat`
      }))
    },
    null,
    2
  ) + "\n"
);

if (target === "release") {
  rmSync(embeddedAssetRoot, { recursive: true, force: true });
  mkdirSync(embeddedAssetRoot, { recursive: true });

  for (const demo of demos) {
    copyFileSync(join(targetOutputRoot, `${demo}.wasm`), join(embeddedAssetRoot, `${demo}.wasm`));
  }

  writeFileSync(
    join(embeddedAssetRoot, "manifest.json"),
    JSON.stringify(
      {
        sourceTarget: target,
        demos: demos.map((name) => ({
          name,
          wasm: `${name}.wasm`
        }))
      },
      null,
      2
    ) + "\n"
  );

  console.log(`Synced embedded wasm assets to ${embeddedAssetRoot}`);
}

console.log(`Wrote ${demos.length} demo modules to ${targetOutputRoot}`);

function getFlagValue(flag) {
  const flagIndex = process.argv.indexOf(flag);
  if (flagIndex === -1 || flagIndex + 1 >= process.argv.length) {
    return null;
  }
  return process.argv[flagIndex + 1];
}
