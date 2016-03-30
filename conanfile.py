from conans import ConanFile
import os

class VenturaConan(ConanFile):
    name = "ventura"
    version = "0.2"
    generators = "cmake"
    requires = "silicium/0.10@TyRoXx/master"
    url="http://github.com/TyRoXx/ventura"
    license="MIT"
    exports="ventura/*"

    def package(self):
        self.copy(pattern="*.hpp", dst="include/ventura", src="ventura", keep_path=True)

    def imports(self):
        self.copy("*.dll", dst="bin", src="bin")
        self.copy("*.dylib*", dst="bin", src="lib")
