import os
from conan import ConanFile
from conan.tools.build import check_min_cppstd
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout, CMakeDeps
from conan.tools.scm import Git
from conan.tools.files import download, unzip

required_conan_version = ">=2.0"

class DPPConan(ConanFile):
    name = "dpp"
    version = "10.0.34"
    license = "Apache-2.0"
    url = "https://github.com/brainboxdotcc/DPP"
    description = "D++ is a lightweight and efficient library for Discord"
    topics = ("discord")
    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False], "fPIC": [True, False]}
    default_options = {"shared": True, "fPIC": True}

    @property
    def _min_cppstd(self):
        return 17

    def validate(self):
        if self.settings.compiler.get_safe("cppstd"):
            check_min_cppstd(self, self._min_cppstd)

    def requirements(self):
        self.requires("nlohmann_json/3.11.2")
        self.requires("openssl/3.1.2")
        self.requires("zlib/1.3")
        self.requires("opus/1.4")

    def config_options(self):
        if self.settings.os == "Windows":
            self.options.rm_safe("fPIC")

    def configure(self):
        if self.options.shared:
            self.options.rm_safe("fPIC")

    def layout(self):
        cmake_layout(self)

    def export(self):
        git = Git(self, self.recipe_folder)
        git.coordinates_to_conandata()

    def source(self):
        # This environment variable should only be set by D++ library developers to ensure that conan builds succeed
        # without having to wait for release. It will check out the development branch where conanfile.py is being
        # developed and tested. If you are NOT sure what this does, DO NOT SET IT. You won't get the D++ release
        # you expect!
        if 'DPP_CONAN_TESTING' in os.environ:
            git = Git(self)
            git.clone(url="https://github.com/brainboxdotcc/DPP.git", target=".")
            git.checkout(commit="conan-the-librarian")
            zip_name = "DPP.zip"
        else:
            download(self, f"https://github.com/brainboxdotcc/DPP/archive/refs/tags/v{self.version}.zip", zip_name)
            unzip(self, zip_name, '.', False, None, True)
            os.unlink(zip_name)

    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()
        tc = CMakeToolchain(self)
        tc.cache_variables["CONAN_EXPORTED"] = True
        tc.cache_variables["BUILD_VOICE_SUPPORT"] = True
        tc.cache_variables["DPP_BUILD_TEST"] = False
        tc.cache_variables["BUILD_SHARED_LIBS"] = True
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.libs = ["dpp"]
        self.cpp_info.includedirs = ["include/dpp-10.0"]
        self.cpp_info.libdirs = ["lib/dpp-10.0"]
