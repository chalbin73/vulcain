workspace("vulcain")
configurations({"Debug", "Release"})
location("build")

project("vulcain")
kind("ConsoleApp")
language("C")
targetdir("bin")
buildoptions({"-Wall", "-Werror"})
links({"glfw", "m", "pthread", "vulkan", "dl", "rt", "X11", "Xxf86vm", "Xrandr", "Xi"})

files({"src/**.c","src/**.h"})