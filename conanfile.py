from conans import ConanFile, CMake, tools

class DevNewConan(ConanFile):
    name = "cpp_dev_new"
    version = "0.1"
    license = "The Unlicense"
    url = "https://github.com/sorf/cpp-dev-new"
    description = "C++ memory allocator for development and testing "
    settings = "os", "arch", "compiler", "build_type"
    options = {"clang_tidy": [True, False]}
    generators = "cmake"
    exports_sources = "source/*", ".clang-format", ".clang-tidy", "CMakeLists.txt", "format_check.sh"
    requires = "boost/1.69.0@conan/stable"
    build_requires = "catch2/2.4.0@bincrafters/stable"
    default_options = "clang_tidy=False", "boost:header_only=True"

    def build(self):
        cmake = CMake(self)
        cmake.verbose = True
        if self.settings.compiler == "Visual Studio":
            cmake.definitions["CONAN_CXX_FLAGS"] += " /W4 /WX /D_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS" +\
                                                    " /D_WIN32_WINNT=0x0A00" +\
                                                    " /DBOOST_ASIO_STANDALONE"
        elif self.settings.compiler == "gcc":
            cmake.definitions["CONAN_CXX_FLAGS"] += " -Wall -Wextra -Werror -DBOOST_ASIO_STANDALONE"
        elif self.settings.compiler == "clang":
            cmake.definitions["CONAN_CXX_FLAGS"] += " -Wall -Wextra -Werror -Wglobal-constructors -DBOOST_ASIO_STANDALONE"

        if not self.settings.os == "Windows":
            cmake.definitions["CONAN_CXX_FLAGS"] += " -pthread"

        if self.options.clang_tidy:
            # If testing with clang_tidy, removing the CONAN_LIBCXX flag so that we do not get a
            # clang-tidy warning/error from _GLIBCXX_USE_CXX11_ABI being defined
            del cmake.definitions["CONAN_LIBCXX"]

            path_clang_tidy = tools.which(tools.get_env("CLANG_TIDY", "clang-tidy"))
            if path_clang_tidy:
                cmake.definitions["CLANG_TIDY_COMMAND"] = path_clang_tidy

        cmake.configure()
        cmake.build()
        if self.settings.compiler == "Visual Studio" and self.settings.build_type == "Debug":
            self.output.warn("Skipping tests in Visual Studio/Debug build. "
                             "Error testing will fail with checked iterators")
        else:
            with tools.environment_append({"CTEST_OUTPUT_ON_FAILURE": "1"}):
                cmake.test()

    def package(self):
        self.copy("*.hpp", dst="include", src="source/include")
        self.copy("*.lib", dst="lib", keep_path=False)
        self.copy("*.a", dst="lib", keep_path=False)

    def package_info(self):
        self.cpp_info.libs = ["dev_new"]
