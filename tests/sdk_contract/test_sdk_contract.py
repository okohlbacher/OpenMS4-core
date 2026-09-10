"""Exercise SDK configuration without configuring or compiling OpenMS itself."""
import re
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
CMAKE = shutil.which("cmake")


@unittest.skipUnless(CMAKE, "CMake is required for SDK configuration tests")
class SDKContractTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix="openms-sdk-contract-")
        self.directory = Path(self.temporary.name)
        self.prefix = self.directory / "relocated-sdk"
        self.config = self.prefix / "lib/cmake/OpenMS"
        self.config.mkdir(parents=True)
        (self.prefix / "include").mkdir()
        self.dependencies = self.directory / "dependencies"
        self.dependencies.mkdir()
        self._dependency("CURL", "8.0.0", "CURL::libcurl")
        self._dependency("Boost", "1.90.0", "Boost::boost", "Boost::regex")
        self._dependency("Eigen3", "3.4.0", "Eigen3::Eigen")
        self._dependency("Arrow", "23.0.0", "Arrow::arrow_shared")
        self._dependency("Parquet", "23.0.0", "Parquet::parquet_shared")
        self._generate_config()
        (self.config / "OpenMSTargets.cmake").write_text('''
foreach(name Core OpenSwathAlgo Arrow)
  if(NOT TARGET OpenMS::${name})
    add_library(OpenMS::${name} INTERFACE IMPORTED)
    set_target_properties(OpenMS::${name} PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${PACKAGE_PREFIX_DIR}/include")
  endif()
endforeach()
''')

    def tearDown(self):
        self.temporary.cleanup()

    def _run(self, arguments, cwd=None):
        return subprocess.run([CMAKE, *map(str, arguments)], cwd=cwd, text=True,
                              stdout=subprocess.PIPE, stderr=subprocess.STDOUT)

    def _dependency(self, name, version, *targets):
        directory = self.dependencies / name
        directory.mkdir(exist_ok=True)
        config = f'set({name}_VERSION "{version}")\nset(PACKAGE_PREFIX_DIR "{directory}")\n'
        for target in targets:
            config += f'if(NOT TARGET {target})\n  add_library({target} INTERFACE IMPORTED)\nendif()\n'
        (directory / f"{name}Config.cmake").write_text(config)
        script = self.directory / "dependency-version.cmake"
        script.write_text(f'''include(CMakePackageConfigHelpers)
write_basic_package_version_file("{directory}/{name}ConfigVersion.cmake"
 VERSION {version} COMPATIBILITY ExactVersion ARCH_INDEPENDENT)
''')
        result = self._run(["-P", script])
        self.assertEqual(result.returncode, 0, result.stdout)

    def _generate_config(self):
        values = {"OPENMS_PACKAGE_VERSION": "4.0.0", "OPENMS_SOURCE_REVISION": "a" * 40,
                  "Arrow_VERSION": "23.0.0", "Parquet_VERSION": "23.0.0",
                  "Boost_VERSION_STRING": "1.90.0", "OPENMS_ARROW_TARGET": "Arrow::arrow_shared",
                  "OPENMS_PARQUET_TARGET": "Parquet::parquet_shared", "OPENMP_FOUND": "OFF",
                  "CMAKE_INSTALL_PREFIX": str(self.prefix), "INSTALL_SHARE_DIR": "share/OpenMS/4.0.0",
                  "INSTALL_LIB_DIR": "lib", "INSTALL_BIN_DIR": "bin", "INSTALL_DOC_DIR": "share/doc",
                  "WITH_HDF5": "ON", "WITH_OPENTIMS": "OFF", "WITH_THERMO_RAW": "OFF",
                  "WITH_ONNX": "OFF", "ENABLE_TDL": "OFF", "WITH_WNETALIGN": "OFF",
                  "OPENMS_WITH_OPENSWATH": "ON", "BUILD_SHARED_LIBS": "ON",
                  "ENABLE_CLASS_TESTING": "ON", "CMAKE_CXX_COMPILER_ID": "MockCompiler",
                  "CMAKE_CXX_COMPILER_VERSION": "1.0", "CMAKE_BUILD_TYPE": "Debug"}
        script = self.directory / "generate-config.cmake"
        script.write_text("include(CMakePackageConfigHelpers)\n" +
                          "\n".join(f'set({key} "{value}")' for key, value in values.items()) + f'''
configure_package_config_file("{ROOT}/cmake/OpenMSConfig.cmake.in" "{self.config}/OpenMSConfig.cmake"
 INSTALL_DESTINATION lib/cmake/OpenMS
 PATH_VARS INSTALL_SHARE_DIR INSTALL_LIB_DIR INSTALL_BIN_DIR INSTALL_DOC_DIR)
write_basic_package_version_file("{self.config}/OpenMSConfigVersion.cmake"
 VERSION 4.0.0 COMPATIBILITY ExactVersion ARCH_INDEPENDENT)
''')
        result = self._run(["-P", script])
        self.assertEqual(result.returncode, 0, result.stdout)

    def _consumer(self, extra="", components="Core", version="4.0.0"):
        directory = self.directory / "consumer"
        directory.mkdir(exist_ok=True)
        (directory / "CMakeLists.txt").write_text(f'''cmake_minimum_required(VERSION 3.21)
project(SDKContract LANGUAGES NONE)
set(CMAKE_FIND_USE_PACKAGE_REGISTRY FALSE)
set(CMAKE_FIND_USE_SYSTEM_PACKAGE_REGISTRY FALSE)
set(CMAKE_FIND_PACKAGE_PREFER_CONFIG TRUE)
set(BUILD_TOPP_TOOLS "consumer-tools")
set(WITH_GUI "consumer-gui")
set(WITH_HDF5 "consumer-hdf5")
find_package(OpenMS {version} EXACT CONFIG REQUIRED COMPONENTS {components})
if(NOT BUILD_TOPP_TOOLS STREQUAL "consumer-tools" OR NOT WITH_GUI STREQUAL "consumer-gui" OR NOT WITH_HDF5 STREQUAL "consumer-hdf5")
  message(FATAL_ERROR "SDK overwrote consumer build options")
endif()
if(NOT OpenMS_SOURCE_REVISION STREQUAL "{'a' * 40}" OR NOT OpenMS_CXX_STANDARD EQUAL 23)
  message(FATAL_ERROR "SDK identity was not exported")
endif()
if(NOT OpenMS_DATA_DIR STREQUAL "{self.prefix}/share/OpenMS/4.0.0")
  message(FATAL_ERROR "Data directory is not relative to installed SDK")
endif()
foreach(target OpenMS::Core OpenMS::OpenSwathAlgo OpenMS::Arrow OpenMS OpenSwathAlgo)
  if(NOT TARGET ${{target}})
    message(FATAL_ERROR "Missing expected target ${{target}}")
  endif()
endforeach()
{extra}
''')
        prefixes = [self.prefix, *(self.dependencies / name for name in ("CURL", "Boost", "Eigen3", "Arrow", "Parquet"))]
        return self._run(["-S", directory, "-B", self.directory / "consumer-build",
                          "-DCMAKE_PREFIX_PATH=" + ";".join(map(str, prefixes))])

    def test_library_only_install_and_consumer_options(self):
        result = self._consumer('if(TARGET OpenMS::TestFramework)\nmessage(FATAL_ERROR "Test support leaked")\nendif()')
        self.assertEqual(result.returncode, 0, result.stdout)

    def test_missing_required_test_support_is_rejected(self):
        result = self._consumer(components="Core TestSupport")
        self.assertNotEqual(result.returncode, 0, result.stdout)
        self.assertIn("TestSupport", result.stdout)

    def test_optional_test_support_component(self):
        shutil.copyfile(ROOT / "cmake/OpenMSTestSupportConfig.cmake.in", self.config / "OpenMSTestSupportConfig.cmake")
        (self.config / "OpenMSTestSupportTargets.cmake").write_text("add_library(OpenMS::TestFramework INTERFACE IMPORTED)\n")
        result = self._consumer('''if(NOT TARGET OpenMS::TestFramework OR NOT OpenMS_TEST_DATA_DIR STREQUAL "${OpenMS_DATA_DIR}/test-data/core")
message(FATAL_ERROR "Test support contract missing")
endif()''', components="Core TestSupport")
        self.assertEqual(result.returncode, 0, result.stdout)

    def test_wrong_core_version_is_rejected(self):
        result = self._consumer(version="4.0.1")
        self.assertNotEqual(result.returncode, 0, result.stdout)

    def test_wrong_arrow_version_is_rejected(self):
        self._dependency("Arrow", "24.0.0", "Arrow::arrow_shared")
        result = self._consumer()
        self.assertNotEqual(result.returncode, 0, result.stdout)
        self.assertIn("23.0.0", result.stdout)

    def test_wrong_arrow_linkage_is_rejected(self):
        self._dependency("Arrow", "23.0.0", "Arrow::arrow_static")
        result = self._consumer()
        self.assertNotEqual(result.returncode, 0, result.stdout)
        self.assertIn("arrow_shared", result.stdout)


class OwnershipTests(unittest.TestCase):
    def test_core_has_no_cli_header_dependency(self):
        for directory in (ROOT / "src/openms/source", ROOT / "src/openms/include", ROOT / "src/tests/class_tests/openms/source"):
            for path in directory.rglob("*"):
                if path.suffix not in (".h", ".cpp"):
                    continue
                for name in re.findall(r'#\s*include\s*[<"]OpenMS/APPLICATIONS/([^>"]+)', path.read_text()):
                    self.assertEqual(name, "ConsoleUtils.h", str(path))

    def test_extracted_fixtures_are_local(self):
        data = ROOT / "src/tests/class_tests/openms/data"
        for path in (ROOT / "src/tests/class_tests/openms/source").glob("*.cpp"):
            content = path.read_text()
            self.assertNotIn("../../../topp/", content, str(path))
            for fixture in re.findall(r'OPENMS_GET_TEST_DATA_PATH\("(extracted_topp/[^"]+)"\)', content):
                self.assertTrue((data / fixture).is_file(), fixture)

    @unittest.skipUnless(CMAKE, "CMake is required for source-list verification")
    def test_core_source_lists_resolve(self):
        with tempfile.TemporaryDirectory(prefix="openms-source-list-") as temporary:
            script = Path(temporary) / "verify.cmake"
            script.write_text(f'''function(source_group)
endfunction()
function(set_source_files_properties)
endfunction()
include("{ROOT}/src/openms/includes.cmake")
foreach(path IN LISTS OpenMS_sources OpenMS_sources_h)
  if(NOT EXISTS "{ROOT}/src/openms/${{path}}")
    message(FATAL_ERROR "Core source/header missing: ${{path}}")
  endif()
endforeach()
include("{ROOT}/src/tests/class_tests/openms/executables.cmake")
foreach(name IN LISTS TEST_executables)
  if(NOT EXISTS "{ROOT}/src/tests/class_tests/openms/source/${{name}}.cpp")
    message(FATAL_ERROR "Core test source missing: ${{name}}")
  endif()
endforeach()
''')
            result = subprocess.run([CMAKE, "-P", str(script)], cwd=ROOT / "src/openms",
                                    text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
            self.assertEqual(result.returncode, 0, result.stdout)


@unittest.skipUnless(CMAKE, "CMake is required for data metadata tests")
class DataMetadataTests(unittest.TestCase):
    def test_relocated_metadata_needs_no_native_dependencies(self):
        with tempfile.TemporaryDirectory(prefix="openms-data-metadata-") as temporary:
            directory = Path(temporary)
            original = directory / "original"
            config = original / "lib/cmake/OpenMSData"
            config.mkdir(parents=True)
            data = original / "share/OpenMS/4.0.0"
            data.mkdir(parents=True)
            script = directory / "generate.cmake"
            script.write_text(f'''include(CMakePackageConfigHelpers)
set(CMAKE_INSTALL_PREFIX "{original}")
set(INSTALL_SHARE_DIR "share/OpenMS/4.0.0")
set(OPENMS_PACKAGE_VERSION "4.0.0")
set(OPENMS_SOURCE_REVISION "{'b' * 40}")
set(OPENMP_FOUND ON)
set(OPENMS_WITH_OPENSWATH ON)
configure_package_config_file("{ROOT}/cmake/OpenMSDataConfig.cmake.in"
 "{config}/OpenMSDataConfig.cmake" INSTALL_DESTINATION lib/cmake/OpenMSData
 PATH_VARS INSTALL_SHARE_DIR)
write_basic_package_version_file("{config}/OpenMSDataConfigVersion.cmake"
 VERSION 4.0.0 COMPATIBILITY ExactVersion ARCH_INDEPENDENT)
''')
            result = subprocess.run([CMAKE, "-P", str(script)], capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            relocated = directory / "relocated"
            original.rename(relocated)
            consumer = directory / "consumer"
            consumer.mkdir()
            (consumer / "CMakeLists.txt").write_text(f'''cmake_minimum_required(VERSION 3.24)
project(DataOnly LANGUAGES NONE)
find_package(OpenMSData 4.0.0 EXACT CONFIG REQUIRED ${{REQUEST_COMPONENTS}})
if(NOT OpenMS_DATA_DIR STREQUAL "{relocated}/share/OpenMS/4.0.0")
  message(FATAL_ERROR "Non-relocatable core data")
endif()
if(NOT OpenMSData_SOURCE_REVISION STREQUAL "{'b' * 40}" OR NOT OpenMS_WITH_OPENMP)
  message(FATAL_ERROR "Missing core identity/features")
endif()
if(TARGET OpenMS::Core OR TARGET OpenMP::OpenMP_CXX)
  message(FATAL_ERROR "Data metadata discovered native targets")
endif()
''')
            arguments = [CMAKE, "-S", str(consumer), "-B", str(directory / "build"),
                         f"-DCMAKE_PREFIX_PATH={relocated}"]
            result = subprocess.run(arguments, capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            result = subprocess.run(arguments + ["-DREQUEST_COMPONENTS=COMPONENTS;TestSupport"],
                                    capture_output=True, text=True)
            self.assertNotEqual(result.returncode, 0, "Missing test fixtures must be rejected")
            (relocated / "share/OpenMS/4.0.0/test-data/core").mkdir(parents=True)
            result = subprocess.run(arguments + ["-DREQUEST_COMPONENTS=COMPONENTS;TestSupport"],
                                    capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)


if __name__ == "__main__":
    unittest.main()
