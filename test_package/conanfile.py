import os
from conans import ConanFile, CMake, tools

class DevNewExampleConan(ConanFile):
    name = "dev_new_example"
    settings = "os", "arch", "compiler", "build_type"
    generators = "cmake"

    def build(self):
        cmake = CMake(self)
        cmake.verbose = True

        if self.options["dev_new"].clang_tidy:
            # If testing with clang_tidy, removing the CONAN_LIBCXX flag so that we do not get a
            # clang-tidy warning/error from _GLIBCXX_USE_CXX11_ABI being defined
            del cmake.definitions["CONAN_LIBCXX"]

            path_clang_tidy = tools.which(tools.get_env("CLANG_TIDY", "clang-tidy"))
            if path_clang_tidy:
                cmake.definitions["CLANG_TIDY_COMMAND"] = path_clang_tidy

        cmake.configure()
        cmake.build()

    def test(self):
        if not tools.cross_building(self.settings):
            os.chdir("bin")
            self.run(".%sexample" % os.sep)
