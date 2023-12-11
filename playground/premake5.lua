workspace "vulcain"
configurations { "Debug", "Release" }
location "build"

project "playground"
language "C"
kind "ConsoleApp"
toolset "clang"
targetdir "bin"
includedirs { "third_party/mathc", "third_party/fast_obj", "third_party/stb", "../third_party/vma/include", "../src" }
libdirs { "third_party/stb", "../third_party/vma/lib", "../lib"}
buildoptions { "-fsanitize=address" }
links { "m", "vulcain", "img", "glfw", "vulkan", "vma", "dl", "pthread", "stdc++" , "asan" }

files { "src/**.c", "third_party/fast_obj/**.c", "third_party/mathc/**.c"}
