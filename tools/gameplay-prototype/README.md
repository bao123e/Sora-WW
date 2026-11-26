# Offline gameplay experiment

This prepares a separate experimental copy of the exact owner-supplied
`Aki.Game` assembly, SHA256
`b79969a511de727ae9444382387294e144522e28a433b2e7c1f4b3eceef2a74f`.
The standalone prepare.py never installs into the game, creates a PAK, loads game code, injects a DLL,
contacts a server or changes the source file. Game binaries and resulting
experimental DLLs must stay outside this repository.

The `no-cooldown` experiment intercepts `CharacterSkillCdComponent.IsSkillInCd`
and `StartCd` for role entities whose positive owner ID equals the local
`CreatureModel.GetPlayerId()`. The `ignore-local-damage` experiment guards
`BaseDamageComponent.ProcessDamage` and `CharacterDamageComponent.FallInjure`
with the same predicate. Every other entity follows the original method body.
All original instructions, relative branches, locals and metadata tokens remain
available. Only the MethodDef RVAs and the final PE section's size/lifetime
fields change; new guarded CIL bodies are appended in the separate copy.

These are **offline experiments, not working game features**. No runtime toggle,
loader, signed package deployment, server HP protection or damage multiplier is
implemented. Skill charges, icons, network replication, other incoming damage
paths and actual Mono execution remain unverified. The local return from
`ProcessDamage` is used for toughness reduction; multiplying it is not a valid
implementation of HP damage multiplication.

```sh
python -m pip install -r tools/gameplay-prototype/requirements.txt
python -m unittest discover -s tools/gameplay-prototype -p 'test_*.py'
python tools/gameplay-prototype/prepare.py /private/Aki.Game.dll \
  --feature no-cooldown --feature ignore-local-damage \
  --output /private/Aki.Game.experimental.dll
```

The source hash is mandatory and outputs are created exclusively. Unsupported
inputs, exception sections, existing outputs and signed images are rejected.
The resulting report distinguishes structural validation from game testing.
The guard's null/ownership/role cases are exercised without running game code.

## Integrated file preparer

`trial_backend.py` is bundled as Sora-Gameplay.exe for the experimental menu. It verifies the full known pak18 hash, prepares the separate DLL image in memory, and creates one plaintext v12 overlay PAK at the source-referenced ScriptAssemblies path. The original PAK/SIG/EXE are untouched. It does not change signature checks or force the client to accept the overlay. Whether the real client mounts and executes this package is unverified.

Changes require the game to be closed. The backend keeps a local manifest and removes only an output whose recorded hash still matches. Untracked or externally modified outputs are preserved. Repeating a launch while preparing is blocked by the native menu, and preparer failure prevents launching a game with the requested trial settings silently omitted.

`pak_format.py` checks footer/secondary hashes and reads its package back. Its FNV path-hash test vector comes from the previously verified real pak43 directory index. `test_trial.py` checks corrupted packages, source preservation, owned-output cleanup and interrupted-install handling without real game assets. Full real-input prepare/read-back/removal was separately checked in the private workspace; original binaries and derived DLLs are never published in this repository.
