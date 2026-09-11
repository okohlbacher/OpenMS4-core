"""One real archive round trip protects deployed SDK contents and symlinks."""

import hashlib
import os
from pathlib import Path
import tempfile
import unittest

from run import extract_sdk, package_sdk
from verify_release import PLATFORMS, verify


class DeploymentArchiveTest(unittest.TestCase):
    def test_release_requires_complete_matching_archives(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            sdk = root / "sdk"
            sdk.mkdir()
            revision = "a" * 40
            (sdk / "source-revision.txt").write_text(revision + "\n")
            for platform in PLATFORMS:
                package_sdk(sdk, root / "artifacts" / f"sdk-{platform}", root / "deployed",
                            f"OpenMS4-core-{platform}-Release-{revision[:12]}")
            verify(root / "artifacts", revision)
            with self.assertRaises(ValueError):
                verify(root / "artifacts", "b" * 40)
            with self.assertRaisesRegex(ValueError, "Source revision mismatch"):
                verify(root / "artifacts", revision[:12] + "b" * 28)
            checksum = next((root / "artifacts").rglob("*.sha256"))
            checksum.write_text("wrong checksum\n")
            with self.assertRaisesRegex(ValueError, "Checksum mismatch"):
                verify(root / "artifacts", revision)
            checksum.unlink()
            with self.assertRaisesRegex(ValueError, "exactly five"):
                verify(root / "artifacts", revision)

    def test_archive_round_trip(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            sdk = root / "sdk"
            (sdk / "lib").mkdir(parents=True)
            (sdk / "lib/library.1").write_bytes(b"SDK library bytes\x00\xff")
            (sdk / "dependencies.txt").write_text("@EXPLICIT\n", encoding="utf-8")
            if os.name != "nt":
                (sdk / "lib/library").symlink_to("library.1")
            deployed = package_sdk(sdk, root / "dist", root / "deployed", "test-sdk")
            self.assertEqual((deployed / "lib/library.1").read_bytes(), b"SDK library bytes\x00\xff")
            self.assertEqual((deployed / "dependencies.txt").read_text(), "@EXPLICIT\n")
            if os.name != "nt":
                self.assertTrue((deployed / "lib/library").is_symlink())
                self.assertEqual(os.readlink(deployed / "lib/library"), "library.1")
            archive = root / "dist/test-sdk.tar.gz"
            self.assertEqual(archive.with_suffix(".gz.sha256").read_text(),
                             f"{hashlib.sha256(archive.read_bytes()).hexdigest()}  test-sdk.tar.gz\n")

            archive.write_bytes(archive.read_bytes() + b"corruption")
            with self.assertRaisesRegex(ValueError, "Checksum mismatch"):
                extract_sdk(archive, root / "invalid")
            self.assertFalse((root / "invalid").exists())


if __name__ == "__main__":
    unittest.main()
