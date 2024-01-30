#ifndef __VC_GLFW__
#define __VC_GLFW__

#include "../vc_windowing.h"

#define VC_WS_GLFW
#ifdef VC_WS_GLFW

#include <GLFW/glfw3.h>

vc_windowing_system vc_ws_glfw(GLFWwindow *window);

#endif

#endif // __VC_GLFW__

