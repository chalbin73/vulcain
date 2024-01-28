#ifndef __VC_DEVICE__
#define __VC_DEVICE__

#include "vulcain.h"

/*
 * Device creation is going to be done in a sort of "builder" pattern :
 *
 * Begin
 *
 * Select physical device through score function (user defined)
 *
 * Set surface/windowing system dependent calls
 *
 * Opaque queue request (through creation function)
 *      the user calls a "create queue function"
 *      The queue is actually created at the end of the device creation
 *      and handles that were give are actually filled at this point
 *
 * ...
 *
 * End (everything is created here)
 */

#include "handles/vc_handles.h"


// Gives a score based on the physical device that is then used for physical device selection.
typedef uint64_t (*vc_device_score_function)(void *usr_data, VkPhysicalDevice candidate);

// Struct passed between the stages of creation
typedef void *vc_device_builder; // Pointer to clean namespace, and not have multiple defs

/**
 * @brief Begins the selection/creation process of a device
 *
 * @param ctx The context in which to create a device
 * @return A device builder opaque object used in creation/selection functions
 */
vc_device_builder vc_device_builder_begin(vc_ctx   *ctx);

/**
 * @brief Needs to be called in order to commit the selection and requested queues
 *
 * @param builder The builder on which requests where mase
 */
void              vc_device_builder_end(vc_device_builder    builder);// This would return a device handle if multiple devices are supported in the future.

/**
 * @brief Requests an extension to be supported, and enabled on the device
 *
 * @param builder The device builder
 * @param extension_name The name of the extension to request, null terminated
 */
void              vc_device_builder_request_extension(vc_device_builder builder, const char *extension_name);

/**
 * @brief Sets a score function, that needs to return a score per physical device for selection
 *
 * @param builder The device builder
 * @param func The score function
 * @param usr_data The user data to pass to the score function
 */
void              vc_device_builder_set_score_func(vc_device_builder builder, vc_device_score_function func, void *usr_data);


/**
 * @brief Requests a queue in the device created by the builder
 *
 * @param builder The device builder
 * @param queue_types The supports required by this queue
 * @return A queue handle. Cannot be used before vc_device_create_end
 */
vc_queue          vc_device_builder_add_queue(vc_device_builder builder, VkQueueFlags queue_types);

#endif //__VC_DEVICE__

