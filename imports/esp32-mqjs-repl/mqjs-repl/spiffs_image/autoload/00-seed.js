// Seed file flashed into SPIFFS so `:autoload` loads at least one real script.
// Keep it tiny and deterministic: tests assert on AUTOLOAD_SEED.

globalThis.AUTOLOAD_SEED = 123;
print("autoload seed loaded: AUTOLOAD_SEED=" + globalThis.AUTOLOAD_SEED);

