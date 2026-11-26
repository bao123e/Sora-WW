"""Small plaintext Kuro v12 PAK writer/reader for a single managed assembly.

Independently implemented from observed pak18 entry framing and pak43 indexes.
No signing, decryption or client check changes are performed.
"""
import hashlib
import struct

VIRTUAL_PATH = 'Client/Content/Aki/ScriptAssemblies/Aki.Game.dll'
MAGIC = 0x5a6f12e1


def fstring(value):
    raw = value.encode('ascii') + b'\0'
    return struct.pack('<i', len(raw)) + raw


def path_hash(value, seed=0):
    result = (0xcbf29ce484222325 + seed) & 0xffffffffffffffff
    for byte in value.lower().encode('utf-16le'):
        result = ((result ^ byte) * 0x100000001b3) & 0xffffffffffffffff
    return result


def build(image):
    # In the actual pak18, CustomData 4 entries have 64-byte data alignment.
    size = len(image)
    data = struct.pack('<qqqI', 0, size, size, 0) + hashlib.sha1(image).digest() + b'\x10' + struct.pack('<I', 0) + bytes(11) + image
    encoded = struct.pack('<IBII', 0xe0000000, 4, size, 0)
    phi = struct.pack('<iQiI', 1, path_hash(VIRTUAL_PATH), 0, 0)
    parts = VIRTUAL_PATH.split('/')
    folders = ['/'] + ['/'.join(parts[:i]) + '/' for i in range(1, len(parts))]
    directory = bytearray(struct.pack('<i', len(folders)))
    for folder in sorted(folders):
        leaf = folder == '/'.join(parts[:-1]) + '/'
        directory.extend(fstring(folder) + struct.pack('<i', int(leaf)))
        if leaf:
            directory.extend(fstring(parts[-1]) + struct.pack('<i', 0))
    directory = bytes(directory)

    def primary(phi_offset, dir_offset):
        return (fstring('../../../') + struct.pack('<iQ', 1, 0)
                + struct.pack('<iqq', 1, phi_offset, len(phi)) + hashlib.sha1(phi).digest()
                + struct.pack('<iqq', 1, dir_offset, len(directory)) + hashlib.sha1(directory).digest()
                + struct.pack('<i', len(encoded)) + encoded + struct.pack('<i', 0))
    index_offset = len(data)
    index_size = len(primary(0, 0))
    index = primary(index_offset + index_size, index_offset + index_size + len(phi))
    footer = bytes(17) + struct.pack('<IIqq', MAGIC, 12, index_offset, len(index)) + hashlib.sha1(index).digest() + bytes(160)
    result = data + index + phi + directory + footer
    assert read_image(result) == image
    return result


def read_image(pak):
    """Read back independently; reject corrupt bounds, hashes, names and entry flags."""
    footer = len(pak) - 221
    if footer < 64 or pak[footer:footer + 17] != bytes(17):
        raise ValueError('BAD_FOOTER')
    magic, version, offset, length = struct.unpack_from('<IIqq', pak, footer + 17)
    if magic != MAGIC or version != 12 or offset < 64 or length < 1 or offset + length > footer:
        raise ValueError('BAD_INDEX_BOUNDS')
    index = pak[offset:offset + length]
    if hashlib.sha1(index).digest() != pak[footer + 41:footer + 61]:
        raise ValueError('BAD_INDEX_HASH')
    pos = 0

    def string(data):
        nonlocal pos
        count = struct.unpack_from('<i', data, pos)[0]; pos += 4
        if count < 1 or pos + count > len(data) or data[pos + count - 1] != 0:
            raise ValueError('BAD_STRING')
        value = data[pos:pos + count - 1].decode('ascii'); pos += count
        return value

    if string(index) != '../../../':
        raise ValueError('BAD_MOUNT')
    count, seed = struct.unpack_from('<iQ', index, pos); pos += 12
    if count != 1 or seed != 0:
        raise ValueError('BAD_ENTRY_COUNT')
    secondary = []
    for _ in range(2):
        present, start, size = struct.unpack_from('<iqq', index, pos); pos += 20
        digest = index[pos:pos + 20]; pos += 20
        if present != 1 or start < offset + length or size < 1 or start + size > footer:
            raise ValueError('BAD_SECONDARY_BOUNDS')
        block = pak[start:start + size]
        if hashlib.sha1(block).digest() != digest:
            raise ValueError('BAD_SECONDARY_HASH')
        secondary.append(block)
    encoded_size = struct.unpack_from('<i', index, pos)[0]; pos += 4
    if encoded_size != 13:
        raise ValueError('BAD_ENCODED_SIZE')
    flags, custom, image_size, entry_offset = struct.unpack_from('<IBII', index, pos); pos += 13
    if flags != 0xe0000000 or custom != 4 or entry_offset != 0 or image_size != offset - 64:
        raise ValueError('BAD_ENCODED_ENTRY')
    if struct.unpack_from('<i', index, pos)[0] != 0 or pos + 4 != len(index):
        raise ValueError('BAD_PRIMARY_END')
    if secondary[0] != struct.pack('<iQiI', 1, path_hash(VIRTUAL_PATH), 0, 0):
        raise ValueError('BAD_PATH_HASH')
    directory = secondary[1]; pos = 4
    folder_count = struct.unpack_from('<i', directory)[0]
    if folder_count < 1 or folder_count > 10:
        raise ValueError('BAD_DIRECTORY_COUNT')
    names = []
    for _ in range(folder_count):
        folder = string(directory)
        file_count = struct.unpack_from('<i', directory, pos)[0]; pos += 4
        if file_count not in (0, 1):
            raise ValueError('BAD_FILE_COUNT')
        for _ in range(file_count):
            names.append(folder + string(directory))
            if struct.unpack_from('<i', directory, pos)[0] != 0:
                raise ValueError('BAD_DIRECTORY_ENTRY')
            pos += 4
    if pos != len(directory) or names != [VIRTUAL_PATH]:
        raise ValueError('BAD_FILE_PATH')
    zero, size, unpacked, compression = struct.unpack_from('<qqqI', pak, 0)
    if zero or size != image_size or unpacked != size or compression or pak[48] != 16 or pak[53:64] != bytes(11):
        raise ValueError('BAD_DATA_HEADER')
    image = pak[64:offset]
    if hashlib.sha1(image).digest() != pak[28:48]:
        raise ValueError('BAD_IMAGE_HASH')
    return image
