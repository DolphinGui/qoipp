from conans import ConanFile, CMake


class QoippConan(ConanFile):
    name = "qoipp"
    version = "0.0.1"
    license = "mit"
    author = "Shin Umeda umeda.shin@gmail.com"
    url = "<Package recipe repository url here, for issues about the package>"
    description = "A c++11 implementation of the qoi image format."
    topics = ("qoi")
    exports_sources = "tests/*", "CMakeLists.txt", "include/*", "039.qoi"
    no_copy_source = True

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
        cmake.test()

    def package(self):
        self.copy("*.hpp", dst="include", src="include")
        self.copy("*.ipp", dst="include", src="include")

    def package_info(self):
        self.cpp_info.libs = ["qoipp"]
