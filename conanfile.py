from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain, cmake_layout
from conan.tools.files import copy
import os

class LuminLoggerConan(ConanFile):
    name = "lumin-logger"
    version = "1.0.0"
    license = "MIT"
    author = "Lumin Team <info@lumin.dev>"
    url = "https://github.com/lumin/logger"
    description = "A modern C++ logging library"
    topics = ("logging", "c++", "spdlog", "json")
    settings = "os", "compiler", "build_type", "arch"
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
        "build_examples": [True, False],
        "build_tests": [True, False]
    }
    default_options = {
        "shared": False,
        "fPIC": True,
        "build_examples": False,
        "build_tests": False,
        "spdlog/*:header_only": True
    }
    
    exports_sources = (
        "CMakeLists.txt", "LICENSE", "README.md", "CHANGELOG.md",
        "include/*", "src/*", "cmake/*", "examples/*", "tests/*"
    )
    
    def requirements(self):
        self.requires("spdlog/1.12.0")
        self.requires("nlohmann_json/3.11.3")
    
    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC
    
    def layout(self):
        cmake_layout(self)
    
    def generate(self):
        tc = CMakeToolchain(self)
        tc.variables["LUMIN_LOGGER_BUILD_EXAMPLES"] = self.options.build_examples
        tc.variables["LUMIN_LOGGER_BUILD_TESTS"] = self.options.build_tests
        tc.generate()
    
    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
        if self.options.build_tests:
            cmake.test()
    
    def package(self):
        cmake = CMake(self)
        cmake.install()
        copy(self, "LICENSE", self.source_folder, os.path.join(self.package_folder, "licenses"))
    
    def package_info(self):
        self.cpp_info.libs = ["lumin_logger"]
        self.cpp_info.set_property("cmake_file_name", "lumin-logger")
        self.cpp_info.set_property("cmake_target_name", "lumin::logger")
        self.cpp_info.requires = ["spdlog::spdlog", "nlohmann_json::nlohmann_json"] 