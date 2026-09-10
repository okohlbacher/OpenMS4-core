"""Check dependency selection and native CMake unity handling with tiny fixtures."""
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
CMAKE = shutil.which("cmake")


@unittest.skipUnless(CMAKE, "CMake is required")
class BuildDefaultsTests(unittest.TestCase):
    def run_cmake(self, *arguments):
        result = subprocess.run([CMAKE, *map(str, arguments)], capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_curl_prefers_native_library_and_preserves_explicit_framework_choice(self):
        with tempfile.TemporaryDirectory(prefix="core-dependency-") as directory:
            root = Path(directory).resolve()
            framework = root / "frameworks/libcurl.framework"
            framework.mkdir(parents=True)
            (framework / "libcurl").touch()
            native = root / "prefix/lib/libcurl.dylib"
            native.parent.mkdir(parents=True)
            native.touch()
            for preference, expected in ((None, native), ("FIRST", framework)):
                with self.subTest(preference=preference):
                    setting = f'set(CMAKE_FIND_FRAMEWORK {preference} CACHE STRING "Fixture preference")' if preference else ""
                    script = root / "CMakeLists.txt"
                    script.write_text(f'''cmake_minimum_required(VERSION 3.24)
set(CMAKE_SYSTEM_NAME Darwin)
{setting}
project(CurlChoice LANGUAGES NONE)
include("{ROOT.as_posix()}/cmake/Modules/OpenMSDependencyDefaults.cmake")
set(CMAKE_FIND_LIBRARY_PREFIXES lib)
set(CMAKE_FIND_LIBRARY_SUFFIXES .dylib)
find_library(PROBE NAMES curl libcurl NO_DEFAULT_PATH
  PATHS "{framework.parent.as_posix()}" "{native.parent.as_posix()}")
file(WRITE "${{CMAKE_BINARY_DIR}}/result.txt" "${{PROBE}}")
''')
                    build = root / f"build-{preference}"
                    self.run_cmake("-S", root, "-B", build)
                    self.assertEqual(Path((build / "result.txt").read_text()), expected)

    @unittest.skipUnless(shutil.which("c++") or shutil.which("cl"), "C++ compiler is required")
    def test_unity_build_preserves_excluded_translation_units(self):
        with tempfile.TemporaryDirectory(prefix="core-unity-") as directory:
            root = Path(directory).resolve()
            # Readers require UTF-8 bytes, including for escaped Unicode literals.
            (root / "a.cpp").write_text(
                '#include <string_view>\n'
                'static_assert(std::string_view("\\u2212") == "\\xe2\\x88\\x92");\n'
                'namespace { int local_value = 1; }\nint a() { return local_value; }\n')
            (root / "b.cpp").write_text("int b() { return 2; }\n")
            # This name would collide with a.cpp if the exclusion were ignored.
            (root / "separate.cpp").write_text("namespace { int local_value = 3; }\nint c() { return local_value; }\n")
            (root / "CMakeLists.txt").write_text(f'''cmake_minimum_required(VERSION 3.24)
project(CoreUnity LANGUAGES CXX)
set(BUILD_SHARED_LIBS ON)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
list(APPEND CMAKE_MODULE_PATH "{ROOT.as_posix()}/cmake/Modules")
include("{ROOT.as_posix()}/cmake/compiler_flags.cmake")
include("{ROOT.as_posix()}/cmake/add_library_macros.cmake")
function(install_library)
endfunction()
function(install_headers)
endfunction()
function(openms_register_export_target)
endfunction()
set(OPENMS_HOST_BINARY_DIRECTORY "${{CMAKE_BINARY_DIR}}")
set(INSTALL_INCLUDE_DIR include)
set(ENABLE_UNITYBUILD ON)
set_source_files_properties(separate.cpp PROPERTIES SKIP_UNITY_BUILD_INCLUSION ON)
openms_add_library(TARGET_NAME Fixture SOURCE_FILES a.cpp b.cpp separate.cpp
  DLL_EXPORT_PATH OpenMS/)
''')
            build = root / "build"
            self.run_cmake("-S", root, "-B", build)
            self.run_cmake("--build", build, "--config", "Debug", "--parallel", "2")
            unity_files = list(build.glob("CMakeFiles/Fixture.dir/Unity/*_cxx.cxx"))
            self.assertTrue(unity_files)
            unity_source = "\n".join(path.read_text() for path in unity_files)
            self.assertIn("a.cpp", unity_source)
            self.assertIn("b.cpp", unity_source)
            self.assertNotIn("separate.cpp", unity_source)


if __name__ == "__main__":
    unittest.main()
