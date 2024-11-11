import os
from conan import ConanFile
from conan.errors import ConanInvalidConfiguration
from conan.tools.build import check_min_cppstd
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout, CMakeDeps
from conan.tools.files import get, copy, rmdir
from conan.tools.scm import Version

required_conan_version = ">=1.54"

class DPPConan(ConanFile):
    name = "dpp"
    license = "Apache-2.0"
    package_type = "shared-library"
    url = "https://github.com/conan-io/conan-center-index"
    homepage = "https://github.com/brainboxdotcc/DPP"
    description = "D++ is a lightweight and efficient library for Discord"
    topics = ("discord")
    settings = "os", "compiler", "build_type", "arch"

    @property
    def _min_cppstd(self):
        return 17

    @property
    def _compilers_minimum_version(self):
        return {
            "apple-clang": "14",
            "clang": "10",
            "gcc": "8",
            "msvc": "191",
            "Visual Studio": "16",
        }

    def validate(self):
        if self.settings.compiler.get_safe("cppstd"):
            check_min_cppstd(self, self._min_cppstd)
        minimum_version = self._compilers_minimum_version.get(str(self.settings.compiler), False)
        if minimum_version and Version(self.settings.compiler.version) < minimum_version:
            raise ConanInvalidConfiguration(
                f"{self.ref} requires C++{self._min_cppstd}, which your compiler does not support."
            )

    def requirements(self):
        self.requires("nlohmann_json/3.11.2", transitive_libs=True, transitive_headers=True)
        self.requires("openssl/[>=1.1 <4]")
        self.requires("zlib/[>=1.2.11 <2]")
        self.requires("opus/1.4")

    def layout(self):
        cmake_layout(self, src_folder="src")


    def build_requirements(self):
        self.tool_requires("cmake/[>=3.16 <4]")

    def source(self):
        get(self, **self.conan_data["sources"][self.version], strip_root=True)
  
    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()
        tc = CMakeToolchain(self)
        tc.cache_variables["DPP_NO_VCPKG"] = True
        tc.cache_variables["DPP_USE_EXTERNAL_JSON"] = True
        tc.cache_variables["CONAN_EXPORTED"] = True
        tc.cache_variables["BUILD_VOICE_SUPPORT"] = True
        tc.cache_variables["DPP_BUILD_TEST"] = False
        tc.cache_variables["BUILD_SHARED_LIBS"] = True
        tc.cache_variables["AVX_TYPE"] = "AVX0"
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        copy(self, "LICENSE", self.source_folder, os.path.join(self.package_folder, "licenses"))
        cmake = CMake(self)
        cmake.install()
        rmdir(self, os.path.join(self.package_folder, "lib", "pkgconfig"))
        rmdir(self, os.path.join(self.package_folder, "lib", "cmake"))

    def package_info(self):
        self.cpp_info.libs = ["dpp"]
        self.cpp_info.set_property("cmake_file_name", "dpp")
        self.cpp_info.set_property("cmake_target_name", "dpp::dpp")
        # On windows only, the headers and libs go into dpp-10.0 subdirectories.
        if self.settings.os == "Windows":
            self.cpp_info.includedirs = ["include/dpp-10.0"]
            self.cpp_info.libdirs = ["lib/dpp-10.0"]
        elif self.settings.os in ["Linux", "FreeBSD"]:
            self.cpp_info.system_libs = ["pthread"]
        self.cpp_info.defines = ["DPP_USE_EXTERNAL_JSON"]
