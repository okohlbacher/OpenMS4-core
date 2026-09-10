"""Reject false positive negative tests without building or loading Core."""

from pathlib import Path
import os
import shutil
import subprocess
import sys
import tempfile
import unittest


CMAKE = shutil.which("cmake")
WRAPPER = Path(__file__).resolve().parent / "ExpectInvalidDataOverride.cmake"


@unittest.skipUnless(CMAKE, "CMake is required for the process-result wrapper")
class InvalidDataOverrideWrapperTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix="openms data wrapper ")
        self.directory = Path(self.temporary.name)
        self.override = self.directory / "missing override"
        self.probe = self.directory / "probe.py"

    def tearDown(self):
        self.temporary.cleanup()

    def run_probe(self, body):
        # Controlled child processes reproduce exit/diagnostic/loader outcomes;
        # no installed OpenMS, compiler, shell quoting, or network is involved.
        self.probe.write_text("import os, signal, sys\n" +
                             "override = os.environ['OPENMS_DATA_PATH']\n" +
                             "diagnostic = (\"OpenMS FATAL ERROR!\\n  Cannot find shared data! OpenMS cannot function without it!\\n\"\n"
                             "              + \"  The environment variable 'OPENMS_DATA_PATH' currently points to '\"\n"
                             "              + override + \"', which is incorrect!\\nExiting now.\\n\")\n" + body)
        return subprocess.run(
            [CMAKE, f"-DPROBE_EXECUTABLE={sys.executable}",
             f"-DPROBE_ARGUMENTS={self.probe}", f"-DINVALID_OVERRIDE={self.override}",
             "-P", str(WRAPPER)], capture_output=True, text=True)

    def assert_rejected(self, result):
        self.assertNotEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_accepts_exact_openms_failure(self):
        result = self.run_probe("sys.stderr.write(diagnostic)\nsys.exit(1)\n")
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertIn("exact diagnostic verified", result.stdout)

    def test_rejects_success_even_with_expected_diagnostic(self):
        self.assert_rejected(self.run_probe("sys.stderr.write(diagnostic)\nsys.exit(0)\n"))

    def test_rejects_other_exit_code_even_with_expected_diagnostic(self):
        self.assert_rejected(self.run_probe("sys.stderr.write(diagnostic)\nsys.exit(42)\n"))

    def test_rejects_unrelated_failure(self):
        self.assert_rejected(self.run_probe("sys.stderr.write('an unrelated exception\\n')\nsys.exit(1)\n"))

    def test_rejects_loader_failure(self):
        result = self.run_probe("sys.stderr.write('dyld[123]: Library not loaded: libmissing.dylib\\n')\nsys.exit(1)\n")
        self.assert_rejected(result)
        self.assertIn("runtime loader", result.stderr)

    def test_rejects_loader_failure_mixed_with_expected_diagnostic(self):
        result = self.run_probe("sys.stderr.write(diagnostic + 'dyld[123]: Symbol not found: missing\\n')\nsys.exit(1)\n")
        self.assert_rejected(result)

    def test_rejects_diagnostic_for_different_path(self):
        self.assert_rejected(self.run_probe("sys.stderr.write(diagnostic.replace(override, '/a/different/path'))\nsys.exit(1)\n"))

    def test_rejects_stdout_only_diagnostic(self):
        self.assert_rejected(self.run_probe("sys.stdout.write(diagnostic)\nsys.exit(1)\n"))

    @unittest.skipIf(os.name == "nt", "POSIX signal termination check")
    def test_rejects_signal_even_with_expected_diagnostic(self):
        self.assert_rejected(self.run_probe(
            "sys.stderr.write(diagnostic)\nsys.stderr.flush()\nos.kill(os.getpid(), signal.SIGTERM)\n"))


if __name__ == "__main__":
    unittest.main()
