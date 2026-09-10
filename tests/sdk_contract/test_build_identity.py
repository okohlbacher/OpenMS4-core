"""Exercise source and generated build identity without building OpenMS."""
import json
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
CMAKE = shutil.which("cmake")
GIT = shutil.which("git")


@unittest.skipUnless(CMAKE and GIT, "CMake and Git are required")
class BuildIdentityTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="core-identity-")
        self.directory = Path(self.temp.name)
        self.source = self.directory / "source"
        self.source.mkdir()
        self.run_command([GIT, "init", str(self.source)])
        (self.source / "tracked.txt").write_text("initial\n")
        self.run_command([GIT, "add", "."], self.source)
        self.run_command([GIT, "-c", "user.name=Identity test", "-c", "user.email=test@example.invalid",
                          "commit", "-m", "fixture"], self.source)
        self.revision = self.run_command([GIT, "rev-parse", "HEAD"], self.source).stdout.strip()

    def tearDown(self):
        self.temp.cleanup()

    def run_command(self, command, cwd=None, success=True):
        result = subprocess.run(list(map(str, command)), cwd=cwd, capture_output=True, text=True)
        if success:
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        return result

    def identity(self, settings="", source=None, success=True):
        script = self.directory / "identity.cmake"
        script.write_text(f'''{settings}
include("{ROOT}/cmake/OpenMSSourceIdentity.cmake")
openms_source_identity("{source or self.source}" revision dirty)
file(WRITE "{self.directory}/identity.txt" "${{revision}};${{dirty}}")
''')
        result = self.run_command([CMAKE, "-P", script], success=success)
        if result.returncode:
            return result
        return (self.directory / "identity.txt").read_text().split(";")

    def test_clean_dirty_staged_and_untracked_identity(self):
        self.assertEqual(self.identity(), [self.revision, "OFF"])
        (self.source / "new.cpp").write_text("int newly_compiled_source;\n")
        self.assertEqual(self.identity(), [self.revision, "ON"])
        (self.source / "new.cpp").unlink()
        self.assertEqual(self.identity(), [self.revision, "OFF"])
        (self.source / "tracked.txt").write_text("changed\n")
        self.assertEqual(self.identity(), [self.revision, "ON"])
        self.run_command([GIT, "add", "tracked.txt"], self.source)
        self.assertEqual(self.identity(), [self.revision, "ON"])
        result = self.identity("set(OPENMS_REQUIRE_CLEAN_SOURCE ON)", success=False)
        self.assertIn("rejects source changes", result.stderr)

    def test_archive_requires_explicit_assertions(self):
        archive = self.directory / "archive"
        archive.mkdir()
        revision = 'set(OPENMS_SOURCE_REVISION "' + self.revision + '")'
        self.assertNotEqual(self.identity(revision, archive, False).returncode, 0)
        self.assertEqual(self.identity(revision + "\nset(OPENMS_SOURCE_DIRTY OFF)", archive),
                         [self.revision, "OFF"])
        result = self.identity('set(OPENMS_SOURCE_REVISION "bad")\nset(OPENMS_SOURCE_DIRTY OFF)',
                               archive, False)
        self.assertIn("40-character", result.stderr)

    def test_override_cannot_relabel_checkout(self):
        result = self.identity('set(OPENMS_SOURCE_REVISION "' + "a" * 40 + '")', success=False)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("disagrees", result.stderr)

    def test_build_guard_rejects_source_changed_after_configuration(self):
        (self.source / "tracked.txt").write_text("late edit\n")
        result = self.run_command([CMAKE, f"-DOPENMS_CHECK_SOURCE_DIR={self.source}",
                                   f"-DOPENMS_EXPECTED_REVISION={self.revision}",
                                   "-DOPENMS_EXPECTED_DIRTY=OFF", "-P",
                                   ROOT / "cmake/OpenMSSourceIdentity.cmake"], success=False)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("rerun CMake", result.stderr)

    @unittest.skipUnless(shutil.which("c++"), "C++ compiler is required")
    def test_standard_library_probe_uses_the_selected_compiler(self):
        project = self.directory / "abi"
        project.mkdir()
        (project / "CMakeLists.txt").write_text(f'''cmake_minimum_required(VERSION 3.24)
project(ABI LANGUAGES CXX)
include("{ROOT}/cmake/OpenMSRuntimeABI.cmake")
file(WRITE "${{CMAKE_BINARY_DIR}}/abi.txt" "${{OPENMS_STANDARD_LIBRARY}};${{OPENMS_LIBSTDCXX_CXX11_ABI}};${{OPENMS_MSVC_RUNTIME_LIBRARY}}")
''')
        self.run_command([CMAKE, "-S", project, "-B", self.directory / "abi-build"])
        library, cxx11, runtime = (self.directory / "abi-build/abi.txt").read_text().split(";")
        self.assertIn(library, ("libc++", "libstdc++", "msvc-stl"))
        if library == "libstdc++":
            self.assertIn(cxx11, ("0", "1"))
        else:
            self.assertEqual(cxx11, "null")
        if library != "msvc-stl":
            self.assertEqual(runtime, "null")

    def test_generated_json_uses_actual_configuration_and_has_no_build_paths(self):
        project = self.directory / "metadata"
        project.mkdir()
        (project / "CMakeLists.txt").write_text(f'''cmake_minimum_required(VERSION 3.24)
project(Identity LANGUAGES NONE)
set(OPENMS_HOST_DIRECTORY "{self.source}")
set(OPENMS_HOST_BINARY_DIRECTORY "${{CMAKE_BINARY_DIR}}")
set(INSTALL_CMAKE_DIR lib/cmake/OpenMS)
set(OPENMS_SOURCE_REVISION "{self.revision}")
set(OPENMS_SOURCE_DIRTY OFF)
set(OPENMS_PACKAGE_VERSION 4.0.0)
set(CMAKE_CXX_COMPILER_ID MockCompiler)
set(CMAKE_CXX_COMPILER_VERSION 1.0)
set(OPENMS_STANDARD_LIBRARY libc++)
set(OPENMS_LIBSTDCXX_CXX11_ABI null)
set(OPENMS_MSVC_RUNTIME_LIBRARY null)
set(BUILD_SHARED_LIBS ON)
set(ENABLE_CLASS_TESTING ON)
set(OPENMS_WITH_OPENSWATH ON)
set(WITH_OPENTIMS OFF)
set(OPENMP_FOUND ON)
set(OPENMS_STL_DEBUG_ENABLED ON)
set(Arrow_VERSION 25.0.0)
set(Parquet_VERSION 25.0.0)
set(Boost_VERSION_STRING 1.90.0)
set(Eigen3_VERSION 5.0.1)
set(CURL_VERSION_STRING 8.15.0)
set(OPENMS_ARROW_TARGET Arrow::arrow_shared)
set(OPENMS_PARQUET_TARGET Parquet::parquet_shared)
include("{ROOT}/cmake/OpenMSBuildInfo.cmake")
''')
        build = self.directory / "build"
        self.run_command([CMAKE, "-S", project, "-B", build, "-DCMAKE_BUILD_TYPE=Debug"])
        text = (build / "identity/Debug/OpenMSBuildInfo.json").read_text()
        info = json.loads(text)
        self.assertEqual(info["source_revision"], self.revision)
        self.assertIs(info["source_dirty"], False)
        self.assertEqual(info["build_type"], "Debug")
        self.assertEqual(info["standard_library"], "libc++")
        self.assertIsNone(info["libstdcxx_cxx11_abi"])
        self.assertIsNone(info["msvc_runtime_library"])
        self.assertIs(info["class_testing_enabled"], True)
        self.assertIs(info["stl_debug"], True)
        self.assertIs(info["features"]["opentims"], False)
        self.assertEqual(info["dependencies"]["curl"]["version"], "8.15.0")
        self.assertNotIn(str(self.directory), text)
        self.assertIn(text.rstrip(), (build / "identity/Debug/OpenMS/openms_build_info.h").read_text())
        if shutil.which("c++"):
            translation = build / "metadata.cpp"
            translation.write_text('#include "identity/Debug/OpenMS/openms_build_info.h"\n#include <string>\nstd::string info = OPENMS_BUILD_INFO;\n')
            self.run_command([shutil.which("c++"), "-std=c++23", "-fsyntax-only", translation])
        self.run_command([CMAKE, "-S", project, "-B", build, "-DCMAKE_BUILD_TYPE=Release"])
        release = json.loads((build / "identity/Release/OpenMSBuildInfo.json").read_text())
        self.assertEqual(release["build_type"], "Release")
        self.assertIs(release["stl_debug"], False)


if __name__ == "__main__":
    unittest.main()
