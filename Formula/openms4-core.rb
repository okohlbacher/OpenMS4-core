class Openms4Core < Formula
  desc "Core C++ SDK for mass-spectrometry software"
  homepage "https://github.com/okohlbacher/OpenMS4-core"
  url "https://github.com/okohlbacher/OpenMS4-core/archive/refs/tags/core-v4.0.0-ci.2.tar.gz"
  # The tag name is not a bare version, so Homebrew would otherwise detect "1".
  version "4.0.0-ci.2"
  sha256 "9103ac2ee20a44d13f97a3436a814d0a036eb80df447461ed37cb03969f02b59"
  license "BSD-3-Clause"

  depends_on "cmake" => :build
  depends_on "ninja" => :build
  depends_on "pkgconf" => :build
  depends_on "apache-arrow"
  depends_on "boost"
  depends_on "cbc"
  depends_on "curl"
  depends_on "eigen"
  depends_on "libomp"
  depends_on "libsvm"
  depends_on "libxml2"
  depends_on "libzip"
  depends_on "xerces-c"

  def install
    prefixes = %w[apache-arrow boost cbc curl eigen libomp libsvm libxml2 libzip xerces-c]
               .map { |name| formula_opt_prefix(name) }
    args = std_cmake_args + %W[
      -G Ninja
      -DENABLE_CLASS_TESTING=OFF
      -DOPENMS_BUILD_TEST_SUPPORT=ON
      -DWITH_OPENTIMS=OFF
      -DWITH_THERMO_RAW=OFF
      -DWITH_WNETALIGN=OFF
      -DENABLE_TDL=OFF
      -DWITH_ONNX=OFF
      -DWITH_HDF5=OFF
      -DLP_SOLVER=COIN
      -DBOOST_USE_STATIC=OFF
      -DCMAKE_PREFIX_PATH=#{prefixes.join(";")}
      -DCURL_ROOT=#{formula_opt_prefix("curl")}
      -DOpenMP_ROOT=#{formula_opt_prefix("libomp")}
      -DCMAKE_FIND_FRAMEWORK=LAST
      -DOPENMS_SOURCE_REVISION=bc9cc12514c768385ce121d6ca4bb710fe1983c4
      -DOPENMS_SOURCE_DIRTY=OFF
      -DOPENMS_REQUIRE_CLEAN_SOURCE=ON
    ]
    system "cmake", "-S", ".", "-B", "build", *args
    system "cmake", "--build", "build", "--parallel", ENV.make_jobs
    system "cmake", "--install", "build"
    # Core links its vendored SQLite statically and no exported target or public
    # header refers to it, but the installed copies collide with Homebrew's sqlite
    # on Linux, where that formula is not keg-only.
    rm_f [include/"sqlite3.h", lib/"libsqlite3.a"]
  end

  test do
    assert_predicate include/"OpenMS/CONCEPT/VersionInfo.h", :exist?
    assert_predicate lib/"libOpenMS.dylib", :exist?
    (testpath/"CMakeLists.txt").write <<~CMAKE
      cmake_minimum_required(VERSION 3.24)
      project(openms4_core_formula_test LANGUAGES CXX)
      find_package(OpenMS 4.0.0 EXACT CONFIG REQUIRED)
      if(NOT TARGET OpenMS::Core)
        message(FATAL_ERROR "OpenMS::Core was not exported")
      endif()
    CMAKE
    system "cmake", "-S", ".", "-B", "build", "-G", "Ninja",
                    "-DCMAKE_PREFIX_PATH=#{prefix}",
                    "-DOpenMP_ROOT=#{Formula["libomp"].opt_prefix}"
  end
end
