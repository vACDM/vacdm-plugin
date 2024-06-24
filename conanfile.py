from conan.tools.cmake import cmake_layout
from conan import ConanFile


class vACDMRecipe(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeToolchain", "CMakeDeps"

    def requirements(self):
        self.requires("jsoncpp/1.9.5")
        self.requires("libcurl/8.6.0", options={"with_ssl": "schannel"})
        self.requires("geographiclib/2.3")
        self.requires("sqlite3/3.45.3")

    def layout(self):
        cmake_layout(self)
