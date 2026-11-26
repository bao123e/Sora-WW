# Offline gameplay experiment

This prepares a separate experimental copy of the exact owner-supplied
`Aki.Game` assembly, SHA256
`b79969a511de727ae9444382387294e144522e28a433b2e7c1f4b3eceef2a74f`.
It never installs into the game, creates a PAK, loads game code, injects a DLL,
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
