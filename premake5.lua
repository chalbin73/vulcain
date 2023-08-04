workspace "vulcain"
configurations { "Debug", "Release" }
location "build"

project "vulcain"
language "C"
kind "StaticLib"
toolset "clang"
targetdir "lib"
buildoptions { "-Wall", "-Werror", "-g" }
includedirs { "third_party/vma/include" }
libdirs { "third_party/vma/lib" }
links { "m", "glfw", "vulkan", "dl", "pthread", "X11", "Xxf86vm", "Xrandr", "Xi"}

files { "src/**.c" }


project "playground"
language "C"
kind "ConsoleApp"
toolset "clang"
targetdir "bin"
includedirs { "playground/third_party/mathc", "playground/third_party/fast_obj", "playground/third_party/stb", "third_party/vma/include", "src" }
libdirs { "playground/third_party/stb", "third_party/vma/lib"}
links { "m", "vulcain", "img", "glfw", "vulkan", "vma", "dl", "pthread", "stdc++" }

files { "playground/src/**.c", "playground/third_party/fast_obj/**.c", "playground/third_party/mathc/**.c"}
