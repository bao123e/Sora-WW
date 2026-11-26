# Sora WW — live No Cooldown alpha

This branch replaces the withdrawn startup-file patch with a live-session controller. Only No Cooldown has a live implementation; God Mode and Damage Multiplier remain unavailable. Real-game effects and embedded Mono compatibility have not been verified.

Open the game normally, start the tool, then press **Enable in session** beside No Cooldown. A supported session must acknowledge `HOOKS_READY` before the menu shows hooks as active. Press **Turn off** to restore original behavior. Selections are not saved across sessions. Failed attachment or an unsupported runtime keeps the feature off; there is no protection bypass or fallback file patch.

The implementation loads our own native bridge through ordinary Windows process rights, uses an already-loaded exported Mono embedding ABI, validates the exact Aki.Game module and original IL, and applies scoped Harmony prefixes. Losing the controller expires a three-second lease. Original PAK/SIG/EXE/DLL files are never changed. The previous owned-overlay removal remains available.

[Vietnamese instructions](README.vi.md) · [Runtime mechanism and validation limits](runtime/README.md)

Build the menu with `python scripts/build.py --zig zig`, the rollback helper with `python scripts/build_backend.py`, and the live backend with `python scripts/build_runtime.py --zig zig`. Windows verification and packaging are in `scripts/verify_runtime_windows.py`, `scripts/verify_windows.py` and `scripts/package.py`. No game assembly or binary is distributed.
