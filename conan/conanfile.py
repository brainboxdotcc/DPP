from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout, CMakeDeps
from conan.tools.scm import Git
from conan.tools.files import load, update_conandata
1
class DPPConan(ConanFile):
    name = "dpp"
    version = "0.1"
    default_user = "brainboxdotcc"
    default_channel = "testing"
    license = "https://github.com/brainboxdotcc/DPP/blob/master/LICENSE"
    url = "https://github.com/brainboxdotcc/DPP"
    description = "D++ is a lightweight and efficient library for Discord"
    topics = ("discord")
    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False], "fPIC": [True, False]}
    default_options = {"shared": False, "fPIC": True}

    def build_requirements(self):
            self.tool_requires("nlohmann_json/3.11.2")
            self.test_requires("openssl/3.1.2")
            self.tool_requires("zlib/1.3")
            self.test_requires("libsodium/cci.20220430")
            self.test_requires("opus/1.4")

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
        scm_url, scm_commit = git.get_url_and_commit()
        update_conandata(self, {"sources": {"commit": scm_commit, "url": scm_url}})

    def source(self):
        git = Git(self)
        sources = self.conan_data["sources"]
        git.clone(url=sources["url"], target=".")
        git.checkout(commit=sources["commit"])

    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()
        tc = CMakeToolchain(self)
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()