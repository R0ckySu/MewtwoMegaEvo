from conan import ConanFile
from conan.tools.cmake import CMakeDeps, CMakeToolchain
import platform

class ProjectMewtwoConan(ConanFile):
    name = "Mewtwo"
    version = "3.0"
    settings = "os", "compiler", "build_type", "arch"
    # generators = "CMakeToolchain", "CMakeDeps"

    def package_info(self):
        if self.settings.os == "Macos":
            self.cpp_info.frameworks = ["Accelerate"]

    def requirements(self):
        # Conditionally include OpenBLAS for non-macOS platforms
        if platform.system() != "Darwin":
            self.requires("openblas/0.3.25")

        self.requires("hdf5/1.14.3")
        self.requires("armadillo/11.4.3")
        self.requires("llvm-openmp/18.1.8")
        self.requires("muparserx/4.0.12")
        self.requires("zlib/1.2.13")
        # self.requires("exprtk/0.0.3")

    def generate(self):
        tc = CMakeToolchain(self)
        tc.generate()

        deps = CMakeDeps(self)
        deps.generate()