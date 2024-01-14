workspace("vulcain")
configurations({"Debug", "Release"})
location("build")

-- Library
project("vulcain")
kind("StaticLib")
language("C")
targetdir("bin")
buildoptions({"-Wall", "-Werror"})
links({"m", "vulkan"})
files({"src/**.c"})

-- Playground
project("vulcain_pg")
kind("ConsoleApp")
language("C")
targetdir("bin")
buildoptions({"-Wall", "-Werror"})
dependson({"libvulcain"})
libdirs({"bin/"})
links({"m", "vulcain", "glfw", "vulkan", "vul})
includedirs({"src/"})
files({"pg/**.c","pg/**.h"})
