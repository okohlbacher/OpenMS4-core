class Openms4Core < Formula
  desc "Core C++ SDK for mass-spectrometry software"
  homepage "https://github.com/okohlbacher/OpenMS4-core"
  url "https://github.com/okohlbacher/OpenMS4-core/archive/refs/tags/core-v4.0.0-ci.5.tar.gz"
  # The tag name is not a bare version, so Homebrew would otherwise detect "1".
  version "4.0.0-ci.5"
  sha256 "5c6efc71022d44ef2dfb0f9984601a3bf03028eb41c399c324a7e951cd4616f3"
  license "BSD-3-Clause"

  bottle do
    root_url "https://github.com/okohlbacher/OpenMS4-core/releases/download/core-v4.0.0-ci.5"
    rebuild 1
    sha256 arm64_sequoia: "46c37886e37d2ca7c72b387e7afbd262c3ef3dcf742b3ec8ffff2cfebab1d5cb"
    sha256 sequoia:       "b6063468eb1e261a3c615afc6a312cf4aeea9ef3ae471cdb8e4dd80b60bb3479"
    sha256 x86_64_linux:  "612f0e16cdca3f7301bfdf1b7afa229413fa55311ae74c423fdcff02eed8ca18"
  end

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
      -DOPENMS_SOURCE_REVISION=ac41cc177023e24a8fbc711a6ce9010187c54c44
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
    assert_predicate lib/shared_library("libOpenMS"), :exist?
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
