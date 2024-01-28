#include "vc_device.h"
#include <alloca.h>
#include "vc_enum_util.h"
#include "handles/vc_internal_types.h"
#include <string.h>


// db prefix stands for device builder
typedef struct
{
    vc_queue        hndl;
    VkQueueFlags    requested_flags;

    // Filled in by the queue allocation system
    u32             familly;
    u32             index;
} _vc_db_queue_request;

typedef struct
{
    vc_ctx                     *ctx;

    _vc_db_queue_request       *queue_requests; // darray
    vc_device_score_function    device_score_function;
    void                       *score_func_usr_data;

    char                      **extension_requests;  // darray
} _vc_db;

// Private prototypes
b8 _vc_device_creation_prepare_queue_ci(VkPhysicalDevice dev, _vc_db_queue_request **requests_darray, VkDeviceQueueCreateInfo **out_infos_darray);
b8 _vc_device_creation_physcial_device_supports_extensions(VkPhysicalDevice device, char **extensions, u32 ext_count);



// Definitions

vc_device_builder
vc_device_builder_begin(vc_ctx   *ctx)
{
    // On heap, object wont live long
    _vc_db *device_builder = mem_allocate(sizeof(_vc_db), MEMORY_TAG_RENDER_DATA);

    device_builder->ctx                = ctx;
    device_builder->queue_requests     = darray_create(_vc_db_queue_request);
    device_builder->extension_requests = darray_create(char *);
    return device_builder;
}

void
vc_device_builder_end(vc_device_builder    builder)
{
    // -- Enumerate all available physical devices
    _vc_db *device_builder = builder;
    vc_trace("Ending and commiting device builder");

    u32 phy_count = 0;
    vkEnumeratePhysicalDevices(device_builder->ctx->vk_instance, &phy_count, NULL);

    if(phy_count == 0)
    {
        vc_fatal("No available Vulkan device found. Aborting device initialization.");
        return;
    }

    VkPhysicalDevice *phy_devices = alloca(sizeof(VkPhysicalDevice) * phy_count);
    vkEnumeratePhysicalDevices(device_builder->ctx->vk_instance, &phy_count, phy_devices);

    vc_info("%d Vulkan physical device(s) found.", phy_count);

    // -- Print available devices
    {
        vc_debug("Available devices :");
        VkPhysicalDeviceProperties prop;
        for(u32 i = 0; i < phy_count; i++)
        {
            vkGetPhysicalDeviceProperties(phy_devices[i], &prop);
            vc_debug("\t[%2d] '%s'", i, prop.deviceName);
        }
    }

    // -- Log requested queues
    {
        vc_debug("Requested queues :");
        vc_debug("\t[id  G|C|T]");
        for(u32 i = 0; i < darray_length(device_builder->queue_requests); i++)
        {
            VkQueueFlags flags = device_builder->queue_requests[i].requested_flags;
            vc_debug(
                "\t[%2i] %c|%c|%c",
                i,
                flags & VK_QUEUE_GRAPHICS_BIT ? 'G' : ' ',
                flags & VK_QUEUE_COMPUTE_BIT ? 'C' : ' ',
                flags & VK_QUEUE_TRANSFER_BIT ? 'T' : ' '
                );
        }
    }

    // -- -- Get rid of devices that can not be used

    // -- Queue support
    {
        vc_debug("Devices that cannot fullfill the requested queues :");
        for(u32 i = 0; i < phy_count; i++)
        {
            b8 supported = _vc_device_creation_prepare_queue_ci(phy_devices[i], &device_builder->queue_requests, NULL);
            if(!supported)
            {
                vc_debug("\t[%2d] Cannot fullfill queues. Discarded.", i);
                phy_devices[i] = VK_NULL_HANDLE;
            }
        }
    }

    // -- Extension support
    {
        vc_debug("Devices that cannot fullfill the requested extensions :");
        for(u32 i = 0; i < phy_count; i++)
        {
            if(phy_devices[i] == VK_NULL_HANDLE)
                continue;

            b8 supported = _vc_device_creation_physcial_device_supports_extensions(
                phy_devices[i],
                device_builder->extension_requests,
                darray_length(device_builder->extension_requests)
                );
            if(!supported)
            {
                vc_debug("\t[%2d] Cannot fullfill extensions. Discarded.", i);
                phy_devices[i] = VK_NULL_HANDLE;
            }
        }
    }

    VkPhysicalDevice selected_phy = VK_NULL_HANDLE;
    // -- Run score function on candidate devices
    if(device_builder->device_score_function)
    {
        vc_debug("Device scores :");
        u32 highest_index = 0;
        u32 prev_high     = 0;

        for(u32 i = 0; i < phy_count; i++)
        {
            if(phy_devices[i] == VK_NULL_HANDLE)
                continue;

            u64 score = device_builder->device_score_function(device_builder->score_func_usr_data, phy_devices[i]);
            vc_debug("\t[%2d] score=%lu", i, score);

            if(score > prev_high)
            {
                highest_index = i;
                prev_high     = score;
            }
        }

        selected_phy = phy_devices[highest_index];
    }
    else
    {
        vc_warn("No device score function provided, selecting first device.");
        for(u32 i = 0; i < phy_count; i++)
        {
            selected_phy = phy_devices[i];
            if(selected_phy != VK_NULL_HANDLE)
            {
                break;
            }
        }
    }

    if(selected_phy == VK_NULL_HANDLE)
    {
        vc_fatal("ERROR: Could not select a physical device. This should not have happened.");
        return;
    }

    // -- Print winner
    {
        VkPhysicalDeviceProperties prop;
        vkGetPhysicalDeviceProperties(selected_phy, &prop);
        vc_info("Selected physical device : '%s'", prop.deviceName);
    }

    // -- -- Logical device creation

    vc_debug("Preparing queue create infos.");
    VkDeviceQueueCreateInfo *queues_ci = darray_create(VkDeviceQueueCreateInfo);
    _vc_device_creation_prepare_queue_ci(selected_phy, &device_builder->queue_requests, &queues_ci);

    VkDeviceCreateInfo device_ci =
    {
        .sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount    = darray_length(queues_ci),
        .pQueueCreateInfos       = queues_ci,
        .enabledLayerCount       = 0,
        .ppEnabledLayerNames     = NULL,
        .enabledExtensionCount   = 0,
        .ppEnabledExtensionNames = NULL,
        .pEnabledFeatures        = NULL,
    };

    VkDevice device = VK_NULL_HANDLE;
    vc_trace("Creating device.");
    VK_CHECK(vkCreateDevice(selected_phy, &device_ci, NULL, &device), "Could not create logical device.");
    vc_info("Device successfully created.");
    device_builder->ctx->current_device = device;
    darray_destroy(queues_ci);

    // -- Pupulate queues
    vc_debug("Retrieve requested queues.");
    for(u32 i = 0; i < darray_length(device_builder->queue_requests); i++)
    {
        VkQueue q;
        vkGetDeviceQueue(device, device_builder->queue_requests[i].familly, device_builder->queue_requests[i].index, &q);
        if(q == VK_NULL_HANDLE)
        {
            vc_fatal("Could not retrieve device queue.");
            return;
        }

        vc_queue hndl                  = device_builder->queue_requests[i].hndl;
        _vc_queue_intern *queue_struct = vc_handles_manager_deref(&device_builder->ctx->handles_manager, hndl);

        queue_struct->queue       = q;
        queue_struct->queue_flags = device_builder->queue_requests->requested_flags;
    }
    vc_debug("Queues retrieved.");

    // -- Cleanup
    darray_destroy(device_builder->queue_requests);

    for(u32 i = 0; i < darray_length(device_builder->extension_requests); i++)
    {
        mem_free(device_builder->extension_requests[i]);
    }
    darray_destroy(device_builder->extension_requests);

    mem_free(device_builder);
    vc_trace("Device creation finished.");
}

/**
 * @brief Returns true if and only if the provided device supports the provided extensions
 *
 * @param device The device on which to check for extension availability
 * @param extensions The list of extension strings
 * @param ext_count The number of extension strings
 * @return Wether or not the device supports all the provided extensions
 */
b8
_vc_device_creation_physcial_device_supports_extensions(VkPhysicalDevice device, char **extensions, u32 ext_count)
{
    u32 device_ext_count = 0;
    vkEnumerateDeviceExtensionProperties(device, NULL, &device_ext_count, NULL);
    VkExtensionProperties *props = alloca(sizeof(VkExtensionProperties) * device_ext_count);
    vkEnumerateDeviceExtensionProperties(device, NULL, &device_ext_count, props);

    for(u32 i = 0; i < ext_count; i++)
    {
        b8 found = FALSE;

        for(u32 j = 0; j < device_ext_count; j++)
        {
            if(strcmp(props[j].extensionName, extensions[i]) == 0)
            {
                found = TRUE;
                j     = device_ext_count;
            }
        }

        if(!found)
        {
            return FALSE;
        }
    }

    return TRUE;
}

#define _VC_QUEUE_FLAGS_COMP(f) \
        ( \
            ( (f)&VK_QUEUE_GRAPHICS_BIT ) ? 2 : 0 + \
            ( (f)&VK_QUEUE_COMPUTE_BIT ) ? 1 : 0 \
        )

/**
 * @brief Sorts a request array by most complex requests
 *
 * @param reqs A darray of queue requests
 * @note Most complex requests will be at lower indices
 */
void
_vc_device_creation_sort_queue_request(_vc_db_queue_request  **reqs)
{
    // Simple selection sort
    u32 len = darray_length(*reqs);
    for(u32 i = 0; i < len; i++)
    {
        // Select maximum
        u32 max_id   = 0;
        i32 max_comp = -I32_MAX;
        for(u32 j = 0; j < len - i; j++)
        {
            i32 comp = _VC_QUEUE_FLAGS_COMP( (*reqs)[j].requested_flags );
            if(comp > max_comp)
            {
                max_comp = comp;
                max_id   = j;
            }
        }

        // Move maximum on top
        _vc_db_queue_request req =
        {
            0
        };
        darray_pop_at(*reqs, max_id, &req);
        darray_push(*reqs, req);
    }
}



/**
 * @brief Checks wether or not a specific physical device can fullfill a request of queues, and prepares queue create infos.
 *
 * @param dev The physical device to check
 * @param requests_darray The array of requests
 * @param out_infos_darray The output darray of create infos. Can be NULL.
 * @return Can it be fullfilled ?
 */
b8
_vc_device_creation_prepare_queue_ci(VkPhysicalDevice dev, _vc_db_queue_request **requests_darray, VkDeviceQueueCreateInfo **out_infos_darray)
{
    u32 qp_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(dev, &qp_count, NULL);
    VkQueueFamilyProperties *qps = alloca( qp_count * sizeof(VkQueueFamilyProperties) );
    u32 *request_count           = alloca( qp_count * sizeof(VkQueueFamilyProperties) );
    mem_memset(request_count, 0, sizeof(VkQueueFamilyProperties) * qp_count);
    vkGetPhysicalDeviceQueueFamilyProperties(dev, &qp_count, qps);

    // Sort by most complex requests
    _vc_device_creation_sort_queue_request(requests_darray);

    /* SORT DEBUG
       {
        vc_debug("Sorted Requested queues :");
        vc_debug("\t[id  G|C|T]");
        for(u32 i = 0; i < darray_length(*requests_darray); i++)
        {
            VkQueueFlags flags = (*requests_darray)[i].requested_flags;
            vc_debug(
                "\t[%2i] %c|%c|%c",
                i,
                flags & VK_QUEUE_GRAPHICS_BIT ? 'G' : ' ',
                flags & VK_QUEUE_COMPUTE_BIT ? 'C' : ' ',
                flags & VK_QUEUE_TRANSFER_BIT ? 'T' : ' '
                );
        }
       }
       SORT DEBUG */

    // Now most demanding requests are down towards i = 0. We fullfill those first :

    u32 len = darray_length(*requests_darray);
    for(u32 i = 0; i < len; i++)
    {
        b8 fullfilled = FALSE;
        // Search a queue that can fullfill :
        for(u32 j = 0; j < qp_count; j++)
        {
            if(
                ( (qps[j].queueFlags & (*requests_darray)[i].requested_flags) == (*requests_darray)[i].requested_flags ) &&
                request_count[j] < qps[j].queueCount
                )
            {
                // Fill in the information for queue retrival latter.
                (*requests_darray)[i].familly = j;
                (*requests_darray)[i].index   = request_count[j];

                // Virtually allocate the queue
                request_count[j]++;
                fullfilled = TRUE;

            }
        }

        if(!fullfilled)
        {
            //One requested queue could not be fullfilled. Aborting
            return FALSE;
        }
    }

    if(!out_infos_darray)
    {
        // Caller of the function just wants to know if requests can be fullfilled. Do not create infos.
        return TRUE;
    }

    // Create createinfos.

    for(u32 i = 0; i < qp_count; i++)
    {
        if(request_count[i] != 0) // Queue was requested in this familly
        {
            static f32 one_priorities[32] =
            {
                0
            }; // TODO: Make this configurable
            VkDeviceQueueCreateInfo ci =
            {
                .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .queueFamilyIndex = i,
                .queueCount       = request_count[i],
                .pQueuePriorities = one_priorities,
            };
            darray_push(*out_infos_darray, ci);
        }
    }

    return TRUE;
}

void
vc_device_builder_request_extension(vc_device_builder builder, const char *extension_name)
{
    _vc_db *device_builder = builder;
    u64 name_length        = strlen(extension_name);
    char *dest             = mem_allocate(name_length + 1, MEMORY_TAG_RENDERER);
    mem_memcpy( dest, (void *)extension_name, name_length );
    dest[name_length] = '\0';

    darray_push(device_builder->extension_requests, dest);
}

void
vc_device_builder_set_score_func(vc_device_builder builder, vc_device_score_function func, void *usr_data)
{
    _vc_db *device_builder = builder;
    device_builder->device_score_function = func;
    device_builder->score_func_usr_data   = usr_data;
}

vc_queue
vc_device_builder_add_queue(vc_device_builder builder, VkQueueFlags queue_types)
{
    _vc_db *device_builder  = builder;
    _vc_db_queue_request rq =
    {
        0
    };

    vc_queue hndl = vc_handles_manager_alloc(&device_builder->ctx->handles_manager, VC_HANDLE_QUEUE);
    rq.hndl            = hndl;
    rq.requested_flags = queue_types;
    darray_push(device_builder->queue_requests, rq);

    return hndl;
}

