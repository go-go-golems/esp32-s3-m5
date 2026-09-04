// ---------------------------------------------------------------- boot --

print('PULP OS v2 booting, abi v' + abiVersion());
M = storeGet('margin', 40);
seedApps();
home();
// SD manifests load async after first paint; rebuild home when they land.
scanApps(function () { if (RUN.id === 'home') { home(); } });
