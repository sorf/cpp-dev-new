from conans import ConanFile, CMake, tools

class DevNewConan(ConanFile):
    name = "dev_new"
    version = "0.1"
    license = "The Unlicense"
    url = "https://github.com/sorf/cpp-dev-new"
    description = "C++ memory allocator for development and testing "
    settings = "os", "arch", "compiler", "build_type"
    generators = "cmake"
    exports_sources = "source/*", ".clang-format", ".clang-tidy", "CMakeLists.txt", "format_check.sh"
    requires = "boost/1.68.0@conan/stable"
    default_options = "boost:header_only=True"

    def _configure_cmake(self):
        cmake = CMake(self)
        cmake.verbose = True
        if self.settings.compiler == "Visual Studio":
            cmake.definitions["CONAN_CXX_FLAGS"] += " /W4 /WX"
        elif self.settings.compiler == "gcc":
            cmake.definitions["CONAN_CXX_FLAGS"] += " -Wall -Wextra -Werror"
        elif self.settings.compiler == "clang":
            cmake.definitions["CONAN_CXX_FLAGS"] += " -Wall -Wextra -Werror -Wglobal-constructors"
            if self.settings.compiler.version == '6.0':
                path_clang_tidy = tools.which('clang-tidy-6.0')
                if path_clang_tidy:
                    cmake.definitions["CLANG_TIDY_COMMAND"] = path_clang_tidy

        cmake.configure()
        return cmake

    def build(self):
        cmake = self._configure_cmake()
        cmake.build()

    def package(self):
        self.copy("*.hpp", dst="include", src="source/include")
        self.copy("*.lib", dst="lib", keep_path=False)
        self.copy("*.a", dst="lib", keep_path=False)

    def package_info(self):
        self.cpp_info.libs = ["dev_new"]
