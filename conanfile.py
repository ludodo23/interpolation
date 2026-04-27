from conan import ConanFile
from conan.tools.files import copy
from conan.tools.cmake import cmake_layout, CMake, CMakeToolchain
from conan.tools.build import check_min_cppstd
import os
import re
from pathlib import Path


class InterpolationConan(ConanFile):
    name = "interpolation"
    package_type = "header-library"

    settings = "os", "arch", "compiler", "build_type"

    exports_sources = "include/*", "tests/*"
    no_copy_source = True

    generators = "CMakeDeps"

    # -------------------------
    # OPTIONS
    # -------------------------
    options = {
        "tracy": [True, False],
        "asan": [True, False],
        "coverage": [True, False],
        "perf": [True, False],
    }

    default_options = {
        "tracy": False,
        "asan": False,
        "coverage": False,
        "perf": False,
    }

    def set_version(self):
        header = Path(self.recipe_folder) / "include" / "interpolation.hpp"
        content = header.read_text()

        version = re.search(r'INTERPOLATION_VERSION\s+"(.+)"', content)
        if version:
            self.version = version.group(1)

    def build_requirements(self):
        self.test_requires("catch2/3.4.0")

    def validate(self):
        check_min_cppstd(self, 14)

    def layout(self):
        cmake_layout(self)

    def generate(self):
        tc = CMakeToolchain(self)

        tc.variables["ENABLE_TRACY"] = self.options.tracy
        tc.variables["ENABLE_ASAN"] = self.options.asan
        tc.variables["ENABLE_COVERAGE"] = self.options.coverage
        tc.variables["ENABLE_PERF"] = self.options.perf

        tc.generate()

    def build(self):
        if self.conf.get("tools.build:skip_test", default=False):
            return

        cmake = CMake(self)
        cmake.configure()   # ← plus de build_script_folder, il prend la racine
        cmake.build()

        # ⚠️ On évite CTest pour perf / profiling
        if not self.options.perf:
            cmake.ctest(cli_args=["--output-on-failure"])

    def package(self):
        copy(self, "*.hpp", self.source_folder, self.package_folder)

    def package_info(self):
        self.cpp_info.bindirs = []
        self.cpp_info.libdirs = []

    def package_id(self):
        self.info.clear()