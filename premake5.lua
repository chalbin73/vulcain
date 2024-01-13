workspace("vulcain")
configurations({"Debug", "Release"})
location("build")

-- Library
project("vulcain")
kind("StaticLib")
language("C")
targetdir("bin")
buildoptions({"-Wall", "-Werror"})
links({"m"})
files({"src/**.c","src/**.h"})

-- Playground
project("vulcain_pg")
kind("ConsoleApp")
language("C")
targetdir("bin")
buildoptions({"-Wall", "-Werror"})
dependson({"libvulcain"})
libdirs({"bin/"})
links({"m", "vulcain"})
includedirs({"src/"})
files({"pg/**.c","pg/**.h"})
