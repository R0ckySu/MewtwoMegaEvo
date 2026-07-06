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

    def configure(self):
        # When using an external BLAS/LAPACK (MKL or AOCL), tell Armadillo not
        # to pull OpenBLAS and to expect the symbols from the linked libraries.
        blas = os.environ.get("MEWTWO_BLAS", "").lower()
        if blas in ("mkl", "aocl"):
            # Don't let Armadillo emit -lblas/-llapack or pull OpenBLAS; we define
            # ARMA_USE_BLAS/LAPACK and link the vendor libs ourselves in CMakeLists.
            self.options["armadillo"].use_blas = False
            self.options["armadillo"].use_lapack = False
            self.options["armadillo"].use_wrapper = False

    def requirements(self):
        # Armadillo BLAS/LAPACK backend, selected by MEWTWO_BLAS:
        #   accelerate  -> macOS built-in (Accelerate framework), no conan dep
        #   mkl         -> Intel oneMKL, provided by the OneAPI environment
        #   aocl        -> AMD AOCL (BLIS + libFLAME), linked in CMakeLists.txt
        #   openblas    -> OpenBLAS from conan (default fallback on Linux)
        blas = os.environ.get("MEWTWO_BLAS", "").lower()
        if platform.system() == "Darwin" or blas == "accelerate":
            pass                      # Accelerate framework (see package_info)
        elif blas in ("mkl", "aocl"):
            pass                      # linked from the vendor libs in CMakeLists.txt
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