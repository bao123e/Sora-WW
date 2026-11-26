"""Build an OFFLINE CIL experiment for the exact owner-supplied Aki.Game assembly.

Never installs a DLL/PAK, opens a game process, or contacts a game server.
Original method code is retained as the fallback for every non-owner entity.
"""
from pathlib import Path
import argparse
import hashlib
import json
import struct

SOURCE_SHA256 = 'b79969a511de727ae9444382387294e144522e28a433b2e7c1f4b3eceef2a74f'
SOURCE_MVID = '2ea7a717-61c0-4953-9f6c-13b1ec3564cd'
TOKENS = {
    'entity': 0x0a000100,
    'creature': 0x2b000004,
    'is_role': 0x0601841b,
    'owner_id': 0x06018409,
    'model': 0x0601bab7,
    'self_id': 0x0601ba5a,
}
TARGETS = {
    'no-cooldown': [(0x018c9f, 'CharacterSkillCdComponent.IsSkillInCd', b'\x16\x2a'),
                    (0x018ca3, 'CharacterSkillCdComponent.StartCd', b'\x17\x2a')],
    'ignore-local-damage': [(0x016f3e, 'BaseDamageComponent.ProcessDamage', b'\x22\0\0\0\0\x2a'),
                            (0x01719b, 'CharacterDamageComponent.FallInjure', b'\x2a')],
}


def guard(return_code):
    """Return only for a role owned by this client; otherwise enter original IL.

    No newly added metadata references, locals, network calls or global toggles.
    Labels always carry an empty evaluation stack into the original body.
    """
    code = bytearray()
    labels, fixups = {}, []

    def emit(raw):
        code.extend(raw)

    def call(name, virtual=False):
        emit(bytes([0x6f if virtual else 0x28]) + struct.pack('<I', TOKENS[name]))

    def branch(op, target):
        emit(bytes([op])); fixups.append((len(code), target)); emit(b'\0' * 4)

    def label(name):
        labels[name] = len(code)

    emit(b'\x02'); call('entity')                # entity = component.Entity
    emit(b'\x25'); branch(0x3a, 'entity')       # dup; brtrue
    emit(b'\x26'); branch(0x38, 'original')     # pop; br
    label('entity'); call('creature', True)
    emit(b'\x25'); branch(0x3a, 'creature')
    emit(b'\x26'); branch(0x38, 'original')
    label('creature'); emit(b'\x25'); call('is_role', True)
    branch(0x3a, 'role')
    emit(b'\x26'); branch(0x38, 'original')
    label('role'); call('owner_id', True)
    emit(b'\x25\x16'); branch(0x3d, 'positive')  # owner ID > 0
    emit(b'\x26'); branch(0x38, 'original')
    label('positive'); call('model')
    emit(b'\x25'); branch(0x3a, 'model')
    emit(b'\x26\x26'); branch(0x38, 'original')
    label('model'); call('self_id', True)
    branch(0x40, 'original')                    # bne.un owner != self
    emit(return_code)
    label('original')
    for pos, name in fixups:
        struct.pack_into('<i', code, pos, labels[name] - (pos + 4))
    return bytes(code)


def align(value, boundary):
    return (value + boundary - 1) // boundary * boundary


def prepare(source, features):
    import dnfile
    from dncil.cil.body.reader import read_method_body_from_bytes

    if hashlib.sha256(source).hexdigest() != SOURCE_SHA256:
        raise ValueError('UNSUPPORTED_ASSEMBLY_HASH: no output was prepared')
    pe = dnfile.dnPE(data=source, clr_lazy_load=True)
    if str(pe.net.mdtables.Module.rows[0].Mvid) != SOURCE_MVID:
        raise ValueError('UNSUPPORTED_MVID')
    if pe.net.struct.Flags & 8 or pe.OPTIONAL_HEADER.DATA_DIRECTORY[4].Size:
        raise ValueError('SIGNED_IMAGE_NOT_SUPPORTED')
    section = pe.sections[-1]
    if section.PointerToRawData + section.SizeOfRawData != len(source):
        raise ValueError('OVERLAY_NOT_SUPPORTED')
    data = bytearray(source)
    edits, allowed_edits = [], []
    for feature in sorted(set(features)):
        for rid, name, return_code in TARGETS[feature]:
            method = pe.net.mdtables.MethodDef.rows[rid - 1]
            body = read_method_body_from_bytes(pe.get_data(method.Rva, 65536))
            if body.exception_handlers or body.flags.MoreSects:
                raise ValueError('METHOD_EXCEPTION_SECTIONS_NOT_SUPPORTED: ' + name)
            # Retain every original instruction, relative branch and local signature.
            original = body.raw_bytes[body.header_size:body.header_size + body.code_size]
            prefix = guard(return_code)
            flags = 0x3003 | (0x10 if body.flags.InitLocals else 0)
            local_sig = body.local_var_sig_tok.value if body.local_var_sig_tok else 0
            new_body = struct.pack('<HHII', flags, max(3, body.max_stack), len(prefix) + len(original), local_sig) + prefix + original
            offset = align(len(data), 4)
            data.extend(b'\0' * (offset - len(data)))
            new_rva = section.VirtualAddress + offset - section.PointerToRawData
            data.extend(new_body)
            table_offset = method.struct.get_file_offset()
            struct.pack_into('<I', data, table_offset, new_rva)
            allowed_edits.append((table_offset, table_offset + 4))
            parsed = read_method_body_from_bytes(new_body)
            assert parsed.code_size == len(prefix) + len(original)
            instruction_offsets = {i.offset for i in parsed.instructions}
            for ins in parsed.instructions:
                if ins.offset >= 12 + len(prefix):
                    continue
                if ins.opcode.name in ('br', 'brtrue', 'bgt', 'bne.un'):
                    assert ins.operand in instruction_offsets
            edits.append({'feature': feature, 'method': name, 'token': hex(0x06000000 + rid),
                          'original_body_sha256': hashlib.sha256(body.raw_bytes).hexdigest(),
                          'original_code_sha256': hashlib.sha256(original).hexdigest(),
                          'new_body_sha256': hashlib.sha256(new_body).hexdigest(),
                          'prefix_bytes': len(prefix), 'new_rva': new_rva,
                          'original_fallback_preserved': True})
    raw_size = align(len(data) - section.PointerToRawData, pe.OPTIONAL_HEADER.FileAlignment)
    virtual_size = len(data) - section.PointerToRawData
    data.extend(b'\0' * (section.PointerToRawData + raw_size - len(data)))
    changes = [(section.get_file_offset() + 8, virtual_size),
               (section.get_file_offset() + 16, raw_size),
               (section.get_file_offset() + 36, section.Characteristics & ~0x02000000),
               (pe.OPTIONAL_HEADER.get_field_absolute_offset('SizeOfImage'),
                align(section.VirtualAddress + virtual_size, pe.OPTIONAL_HEADER.SectionAlignment))]
    for offset, value in changes:
        struct.pack_into('<I', data, offset, value)
        allowed_edits.append((offset, offset + 4))
    allowed = {i for start, end in allowed_edits for i in range(start, end)}
    assert all(a == b or i in allowed for i, (a, b) in enumerate(zip(source, data)))
    checked = dnfile.dnPE(data=bytes(data), clr_lazy_load=True)
    assert str(checked.net.mdtables.Module.rows[0].Mvid) == SOURCE_MVID
    assert str(checked.net.mdtables.Assembly.rows[0].Name) == 'Aki.Game'
    for edit in edits:
        method = checked.net.mdtables.MethodDef.rows[(int(edit['token'], 16) & 0xffffff) - 1]
        assert method.Rva == edit['new_rva']
        body = read_method_body_from_bytes(checked.get_data(method.Rva, 65536))
        assert hashlib.sha256(body.raw_bytes).hexdigest() == edit['new_body_sha256']
        fallback = body.raw_bytes[body.header_size + edit['prefix_bytes']:body.header_size + body.code_size]
        assert hashlib.sha256(fallback).hexdigest() == edit['original_code_sha256']
    return bytes(data), edits


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('assembly', type=Path)
    parser.add_argument('--feature', choices=sorted(TARGETS), action='append', required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    if args.output.resolve() == args.assembly.resolve():
        parser.error('The output must be a separate experimental copy.')
    if args.output.exists() or args.output.with_suffix('.json').exists():
        parser.error('Output already exists; no file will be overwritten.')
    source = args.assembly.read_bytes()
    result, edits = prepare(source, args.feature)
    report = {'status': 'OFFLINE_EXPERIMENT_NOT_GAME_VERIFIED', 'source_sha256': SOURCE_SHA256,
              'output_sha256': hashlib.sha256(result).hexdigest(), 'source_mvid': SOURCE_MVID,
              'features': sorted(set(args.feature)), 'methods': edits,
              'installed_into_game': False, 'game_runtime_tested': False,
              'damage_multiplier_implemented': False,
              'limits': ['No runtime toggle or loader implemented.',
                         'Server HP updates and other damage paths are not blocked or validated.',
                         'Existing skills, energy, charges, UI and replication require actual runtime verification.']}
    with args.output.open('xb') as dest:
        dest.write(result)
    with args.output.with_suffix('.json').open('x') as dest:
        json.dump(report, dest, indent=2); dest.write('\n')
    print('Prepared a separate offline experiment; original input unchanged. No game installation or runtime test performed.')


if __name__ == '__main__':
    main()
