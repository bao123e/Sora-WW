# Live No Cooldown alpha

This is a separate mechanism from the withdrawn 0.4 file patch. Nothing in this
directory edits a game PAK, DLL or EXE. The menu launches a session controller;
that controller loads our own native bridge with ordinary Windows process
rights. The bridge requires an already-loaded Mono module exposing the public
embedding ABI. It never loads a replacement Mono, scans game memory, guesses
function addresses or bypasses denied rights. Unsupported environments return a
specific error and leave the feature off.

The managed payload verifies Aki.Game's known module MVID, exact method tokens,
declaring types, signatures and original IL hashes before installing two Harmony
prefixes. Those prefixes suppress cooldown checks/start only for role entities
whose positive owner ID matches CreatureModel.GetPlayerId. All other instances,
disposed entities and failed ownership checks execute the original methods.

The controller maintains a three-second monotonic lease. OFF, lost pipe,
controller termination, menu Exit or a game-session change disables the effect.
No selections are carried into a future game session. The bridge and disabled
hooks stay loaded until game exit; the tool does not attempt unsafe unloading.

Acknowledgment `HOOKS_READY:<checks>:<starts>` means the patches were installed;
the counters report prefix interception, **not** proof that cooldown UI, skill
charges, server validation or actual gameplay have changed. Only No Cooldown is
implemented here. Live God Mode and HP damage multiplication are not implemented.

Verification uses actual Harmony detours in a synthetic managed fixture, plus
the real Windows remote-loader/pipe against our own synthetic executable. Those
tests establish controller mechanics, entity scoping, restoration and lease
expiry. They do not establish compatibility with the owner's embedded Mono or
the real game. Game runtime testing remains false in build metadata.

```
python scripts/build_runtime.py --zig zig --dotnet dotnet
python scripts/verify_runtime_windows.py --zig zig
```

The build uses Lib.Harmony 2.4.2 (MIT). No game code or binary is distributed.
