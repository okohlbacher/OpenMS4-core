"""A candidate formula must neither build an old tag nor accept its bottles."""

from pathlib import Path
import unittest

from prepare_homebrew_formula import prepare_formula


class HomebrewFormulaTests(unittest.TestCase):
    def test_exact_candidate_and_rejection_of_ambiguous_inputs(self):
        original = (Path(__file__).resolve().parents[2] / "Formula/openms4-core.rb").read_text()
        revision, checksum = "a" * 40, "b" * 64
        url = f"https://github.com/okohlbacher/OpenMS4-core/archive/{revision}.tar.gz"
        candidate = prepare_formula(original, revision, url, checksum)
        self.assertIn(f'url "{url}"', candidate)
        self.assertIn(f'sha256 "{checksum}"', candidate)
        self.assertIn(f"-DOPENMS_SOURCE_REVISION={revision}", candidate)
        self.assertNotIn("bottle do", candidate)
        self.assertNotIn("archive/refs/tags/", candidate)
        self.assertIn('depends_on "apache-arrow"', candidate)
        self.assertIn("test do", candidate)
        self.assertIn("bottle do", original)  # the published formula remains untouched
        for bad in (revision[:12], "z" * 40):
            with self.assertRaises(ValueError):
                prepare_formula(original, bad, url, checksum)
        with self.assertRaises(ValueError):
            prepare_formula(original, revision, url.replace(revision, "c" * 40), checksum)
        with self.assertRaises(ValueError):
            prepare_formula(original + '\n  sha256 "duplicate"\n', revision, url, checksum)


if __name__ == "__main__":
    unittest.main()
