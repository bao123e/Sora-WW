import json
import os
from pathlib import Path
import struct
import tempfile
import unittest

from pak_format import build, read_image, path_hash
from trial_backend import PAK_NAME, apply_trial, destination, digest, game_running


class PakTests(unittest.TestCase):
    def test_observed_path_hash_vector(self):
        path = 'ActorRefConfig/Game/Aki/Map/AkiWorld_WP.AkiWorld_WP/102001812.bytes'
        self.assertEqual(path_hash(path, 4172003406), 0xc9ec3e9e657ba765)

    def test_package_round_trip_and_corruption(self):
        image = b'MZ' + bytes(range(256)) * 13
        pak = build(image)
        self.assertEqual(read_image(pak), image)
        for offset in (64, len(pak) - 222, len(pak) - 180):
            corrupted = bytearray(pak); corrupted[offset] ^= 1
            with self.assertRaises((ValueError, struct.error)):
                read_image(corrupted)
        for raw in (b'', pak[:-1], pak[:200]):
            with self.assertRaises((ValueError, struct.error)):
                read_image(raw)


class OwnershipTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix='sora-trial-')
        root = Path(self.temp.name)
        self.exe = root / 'Thử nghiệm 游戏' / 'Client' / 'Binaries' / 'Win64' / 'Client-Win64-Shipping.exe'
        self.exe.parent.mkdir(parents=True); self.exe.write_bytes(b'synthetic')
        self.state = root / 'state'; self.state.mkdir()
        self.target = destination(self.exe); self.target.parent.mkdir(parents=True)
        self.original = self.target.parent / 'pakchunk18-WindowsNoEditor.pak'; self.original.write_bytes(b'original bytes')
        self.manifest = self.state / 'gameplay-trial.json'

    def tearDown(self):
        self.assertEqual(self.original.read_bytes(), b'original bytes')
        self.temp.cleanup()

    def record(self, raw):
        self.target.write_bytes(raw)
        self.manifest.write_text(json.dumps({'schema': 'SORA-WW-OWNED-TRIAL-1', 'exe': str(self.exe), 'destination': str(self.target), 'sha256': digest(raw)}))

    def test_remove_only_recorded_output(self):
        self.record(b'owned output')
        self.assertEqual(apply_trial(self.exe, self.state, [], running=lambda: False), 'REMOVED')
        self.assertFalse(self.target.exists()); self.assertFalse(self.manifest.exists())

    def test_refuse_untracked_or_changed_output(self):
        self.target.write_bytes(b'untracked')
        with self.assertRaisesRegex(ValueError, 'UNOWNED'):
            apply_trial(self.exe, self.state, [], running=lambda: False)
        self.record(b'owned'); self.target.write_bytes(b'changed externally')
        with self.assertRaisesRegex(ValueError, 'UNOWNED'):
            apply_trial(self.exe, self.state, [], running=lambda: False)
        self.assertEqual(self.target.read_bytes(), b'changed externally')

    def test_running_game_blocks_changes(self):
        self.record(b'owned')
        with self.assertRaisesRegex(ValueError, 'CLOSE_GAME_FIRST'):
            apply_trial(self.exe, self.state, [], running=lambda: True)
        self.assertEqual(self.target.read_bytes(), b'owned')

    def test_unsupported_build_never_writes_package(self):
        with self.assertRaisesRegex(ValueError, 'UNSUPPORTED_BUILD'):
            apply_trial(self.exe, self.state, ['no-cooldown'], running=lambda: False)
        self.assertFalse(self.target.exists()); self.assertFalse(self.manifest.exists())

    def test_interrupted_install_is_removable(self):
        self.record(b'old owned output')
        manifest = json.loads(self.manifest.read_text()); manifest['previous_sha256'] = manifest['sha256']; manifest['sha256'] = digest(b'new intended output')
        self.manifest.write_text(json.dumps(manifest))
        self.assertEqual(apply_trial(self.exe, self.state, [], running=lambda: False), 'REMOVED')
        self.assertFalse(self.target.exists())

    def test_real_windows_process_snapshot(self):
        if os.name != 'nt':
            self.skipTest('Windows-only process enumeration')
        self.assertIsInstance(game_running(), bool)


if __name__ == '__main__':
    unittest.main()
