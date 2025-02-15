from conan import ConanFile
import platform

class ProjectMewtwoConan(ConanFile):
    name = "Mewtwo"
    version = "3.0"
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeToolchain", "CMakeDeps"

    def requirements(self):
        # Conditionally include OpenBLAS for non-macOS platforms
        if platform.system() != "Darwin":
            self.requires("openblas/0.3.25")

        self.requires("hdf5/1.14.3")
        self.requires("armadillo/11.4.3")
        self.requires("llvm-openmp/18.1.8")
        self.requires("muparserx/4.0.12")
        self.requires("exprtk/0.0.3")

        # Optional dependencies
        if self.options.get_safe("wt_web_GUI", True):
            self.requires("wt/4.10.1")
            self.requires("boost/1.83.0")

    def configure(self):
        # Define optional dependencies
        self.options["wt_web_GUI"] = [True, False]
        self.default_options = {"wt_web_GUI": False}