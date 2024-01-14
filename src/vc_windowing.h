#ifndef __VC_WINDOWING_H__
#define __VC_WINDOWING_H__

// Will contain everything needed to communicate with a windowing system in order to create surface/swapchain.
typedef struct
{
    char    windowing_system_name[256];
} vc_windowing_system;

#endif //__VC_WINDOWING_H__

