workspace "vulcain"
configurations { "Debug", "Release" }
location "build"

project "vulcain"
language "C"
kind "StaticLib"
toolset "clang"
targetdir "lib"
buildoptions { "-Wall", "-Werror", "-g", "-fsanitize=address" }
includedirs { "third_party/vma/include" }
libdirs { "third_party/vma/lib" }
links { "m", "glfw", "vulkan", "dl", "pthread", "X11", "Xxf86vm", "Xrandr", "Xi", "asan"}

files { "src/**.c" }

