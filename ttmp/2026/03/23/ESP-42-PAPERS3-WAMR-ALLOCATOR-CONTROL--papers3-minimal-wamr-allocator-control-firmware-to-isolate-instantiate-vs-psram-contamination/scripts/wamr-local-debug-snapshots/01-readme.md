# WAMR Loader Debug Snapshots

These files are tracked copies of the local `managed_components/espressif__wasm-micro-runtime`
loader sources modified during `ESP-42`.

They exist because the main repo ignores `managed_components/`, so normal git history does not
preserve those edits directly.

Use `../check_wamr_loader_snapshot_sync.sh` to verify that the tracked snapshots still match the
live local WAMR sources being built by `0082`.
