from conan import ConanFile
from conan.tools.cmake import CMakeDeps, CMakeToolchain
import os
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
        # Armadillo BLAS/LAPACK backend, selected by MEWTWO_BLAS:
        #   accelerate  -> macOS built-in (Accelerate framework), no conan dep
        #   mkl         -> Intel oneMKL, provided by the OneAPI environment
        #   openblas    -> OpenBLAS from conan (default on Linux / AMD)
        blas = os.environ.get("MEWTWO_BLAS", "").lower()
        if platform.system() == "Darwin" or blas == "accelerate":
            pass                      # Accelerate framework (see package_info)
        elif blas == "mkl":
            pass                      # linked from the OneAPI env in CMakeLists.txt
        else:
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