#include "vc_device.h"
#include <alloca.h>
#include "vc_enum_util.h"
#include "handles/vc_internal_types.h"
#include "base/base.h"
#include "base/data_structures/darray.h"
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
    vc_queue                   *presentation_dest;
    vc_windowing_system         win_sys;
} _vc_db;

// Represents a simple system, to allocate queues based on requests
typedef struct
{
    VkPhysicalDevice           physical_device;
    VkQueueFamilyProperties   *props;
    u32                       *allocations;
    u32                        count;
} _vc_queue_allocator;

// Private prototypes
void                _vc_db_queue_allocator_destroy(_vc_queue_allocator   *alloc);
b8                  _vc_db_queue_allocator_alloc(_vc_queue_allocator *alloc, VkQueueFlags required_flags, u32 *family_id, u32 *index);
_vc_queue_allocator _vc_db_queue_allocator_init(VkPhysicalDevice    dev);
void                _vc_db_queue_allocator_produce_ci(_vc_queue_allocator *alloc, VkDeviceQueueCreateInfo **queue_darray);
b8                  _vc_db_queue_allocator_enable_present(_vc_queue_allocator *alloc, VkSurfaceKHR surface, u32 *family, u32 *index);
b8                  _vc_device_creation_physcial_device_supports_extensions(VkPhysicalDevice device, char **extensions, u32 ext_count);
void                _vc_setup_vma(vc_ctx   *ctx);

i32
_vc_device_creation_queue_comp(VkQueueFlags *a, VkQueueFlagBits *b)
{
    i32 comp_a =
        ( (*a) & VK_QUEUE_GRAPHICS_BIT ) ? 2 : 0 +
        ( (*a) & VK_QUEUE_COMPUTE_BIT ) ? 1 : 0;

    i32 comp_b =
        ( (*a) & VK_QUEUE_GRAPHICS_BIT ) ? 2 : 0 +
        ( (*a) & VK_QUEUE_COMPUTE_BIT ) ? 1 : 0;

    return comp_a - comp_b;
}

// Definitions

vc_device_builder
vc_device_builder_begin(vc_ctx   *ctx)
{
    // On heap, object wont live long
    _vc_db *device_builder = mem_allocate(sizeof(_vc_db), MEMORY_TAG_RENDER_DATA);

    device_builder->ctx                = ctx;
    device_builder->queue_requests     = darray_create(_vc_db_queue_request);
    device_builder->extension_requests = darray_create(char *);
    device_builder->presentation_dest  = VC_NULL_HANDLE;
    return device_builder;
}

void
vc_device_builder_end(vc_device_builder    builder)
{
    // -- Enumerate all available physical devices
    _vc_db *device_builder = builder;
    vc_debug(" ####  Device creation  #### ");
    vc_trace("Ending and commiting device builder");

    b8 presentation_requested = device_builder->presentation_dest ? TRUE : FALSE;

    u32 phy_count = 0;
    vkEnumeratePhysicalDevices(device_builder->ctx->vk_instance, &phy_count, NULL);

    if(phy_count == 0)
    {
        vc_fatal("No available Vulkan device found. Aborting device initialization.");
        return;
    }

    VkPhysicalDevice *phy_devices = alloca(sizeof(VkPhysicalDevice) * phy_count);
    vkEnumeratePhysicalDevices(device_builder->ctx->vk_instance, &phy_count, phy_devices);

    // Logging required extensions
    {
        vc_debug("Requested extensions for device :");
        u32 count = darray_length(device_builder->extension_requests);
        for(u32 i = 0; i < count; i++)
        {
            vc_debug("[%2d] '%s'", i, device_builder->extension_requests[i]);
        }
    }

    vc_debug("");
    vc_debug("~ Physical device selection ~");

    // -- Print available devices
    vc_info("%d Vulkan physical device(s) found.", phy_count);
    {
        vc_debug("Available devices :");
        VkPhysicalDeviceProperties prop;
        for(u32 i = 0; i < phy_count; i++)
        {
            vkGetPhysicalDeviceProperties(phy_devices[i], &prop);
            vc_debug("\t[%2d] '%s'", i, prop.deviceName);
        }
    }

    // -- Sort queues in terms of complexity
    darray_qsort(device_builder->queue_requests, _vc_device_creation_queue_comp);

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

    // A dummy surface is created to check for support and delted immediatly after
    VkSurfaceKHR dummy_surface = VK_NULL_HANDLE;
    if(presentation_requested)
    {
        VK_CHECK(
            device_builder->win_sys.create_surface(
                device_builder->ctx->vk_instance,
                device_builder->win_sys.udata,
                NULL,
                &dummy_surface
                ),
            "Could not create dummy check surface"
            );
    }
    // VkSurfaceKHR dummy_surface = device_builder->win_sys


    // -- Queue support
    {
        vc_debug("");
        if(presentation_requested)
            vc_debug("Presentation was requested for windowing system: '%s'", device_builder->win_sys.windowing_system_name);
        vc_debug("Devices which cannot fullfill the requested queues :");
        u32 queue_req_count = darray_length(device_builder->queue_requests);
        for(u32 i = 0; i < phy_count; i++)
        {
            // Virtually allocate queues, to see if some queue requests will not be able to be fullfilled.
            _vc_queue_allocator alloc = _vc_db_queue_allocator_init(phy_devices[i]);
            b8 valid                  = TRUE;
            for(u32 q = 0; q < queue_req_count; q++)
            {
                valid &= _vc_db_queue_allocator_alloc(&alloc, device_builder->queue_requests[q].requested_flags, NULL, NULL);
            }

            if(presentation_requested)
            {
                valid &= _vc_db_queue_allocator_enable_present(&alloc, dummy_surface, NULL, NULL);
            }

            _vc_db_queue_allocator_destroy(&alloc);

            if(!valid)
            {
                phy_devices[i] = VK_NULL_HANDLE; // Get rid of unusable device.
                vc_debug("\t[%2d] cannot fullfill queues", i);
            }
        }
    }

    // -- Extension support
    {
        vc_debug("");
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
    u32 req_count             = darray_length(device_builder->queue_requests);
    _vc_queue_allocator alloc = _vc_db_queue_allocator_init(selected_phy);
    for(u32 i = 0; i < req_count; i++)
    {
        u32 fam_id = 0;
        u32 index  = 0;
        _vc_db_queue_allocator_alloc(&alloc, device_builder->queue_requests[i].requested_flags, &fam_id, &index);
        device_builder->queue_requests[i].familly = fam_id;
        device_builder->queue_requests[i].index   = index;
    }

    u32 present_family = 0;
    u32 present_index  = 0;
    if(presentation_requested)
    {
        _vc_db_queue_allocator_enable_present(&alloc, dummy_surface, &present_family, &present_index);
    }

    VkDeviceQueueCreateInfo *queues_ci = darray_create(VkDeviceQueueCreateInfo);
    _vc_db_queue_allocator_produce_ci(&alloc, &queues_ci);

    _vc_db_queue_allocator_destroy(&alloc);

    VkPhysicalDeviceDynamicRenderingFeatures feat =
    {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES,
        .dynamicRendering = VK_TRUE,
    };

    VkDeviceCreateInfo device_ci =
    {
        .sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &feat,
        .queueCreateInfoCount    = darray_length(queues_ci),
        .pQueueCreateInfos       = queues_ci,
        .enabledLayerCount       = 0,
        .ppEnabledLayerNames     = NULL,
        .enabledExtensionCount   = darray_length(device_builder->extension_requests),
        .ppEnabledExtensionNames = (const char **)device_builder->extension_requests,
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

        queue_struct->queue              = q;
        queue_struct->queue_flags        = device_builder->queue_requests[i].requested_flags;
        queue_struct->queue_family_index = device_builder->queue_requests[i].familly;

        // Set presentation queue
        if(
            device_builder->queue_requests[i].index == present_index &&
            device_builder->queue_requests[i].familly == present_family &&
            presentation_requested
            )
        {
            *device_builder->presentation_dest = hndl;
            device_builder->presentation_dest  = NULL; // Signifies that presentation queue was found.
        }
    }

    if(presentation_requested && device_builder->presentation_dest != NULL)
    {
        // this means the presentation queue was not filled in the previous loop, meaning a new handle is to be made
        vc_queue pres                 = vc_handles_manager_alloc(&device_builder->ctx->handles_manager, VC_HANDLE_QUEUE);
        _vc_queue_intern *pres_struct = vc_handles_manager_deref(&device_builder->ctx->handles_manager, pres);

        VkQueue q = VK_NULL_HANDLE;

        vkGetDeviceQueue(device, present_family, present_index, &q);

        pres_struct->queue                 = q;
        pres_struct->queue_family_index    = present_family;
        pres_struct->queue_flags           = 0; // Presentation
        *device_builder->presentation_dest = pres;
    }

    vc_debug("Queues retrieved.");

    device_builder->ctx->current_physical_device = selected_phy;
    device_builder->ctx->current_device          = device;

    // -- Cleanup
    darray_destroy(device_builder->queue_requests);

    for(u32 i = 0; i < darray_length(device_builder->extension_requests); i++)
    {
        mem_free(device_builder->extension_requests[i]);
    }
    darray_destroy(device_builder->extension_requests);

    if(dummy_surface != VK_NULL_HANDLE)
    {
        vkDestroySurfaceKHR(device_builder->ctx->vk_instance, dummy_surface, NULL);
    }

    mem_free(device_builder);
    vc_trace("Device creation finished.");
    vc_debug("Create vulkan memory allocator");
    _vc_setup_vma(device_builder->ctx);
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

void
vc_device_builder_request_presentation_support(vc_device_builder builder, vc_queue *queue, vc_windowing_system windowing_system)
{
    _vc_db *device_builder = builder;
    device_builder->presentation_dest = queue;
    device_builder->win_sys           = windowing_system;
    vc_device_builder_request_extension(builder, "VK_KHR_swapchain");
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

_vc_queue_allocator
_vc_db_queue_allocator_init(VkPhysicalDevice    dev)
{
    _vc_queue_allocator alloc;
    alloc.physical_device = dev;

    vkGetPhysicalDeviceQueueFamilyProperties(alloc.physical_device, &alloc.count, NULL);


    alloc.props       = mem_allocate(sizeof(VkQueueFamilyProperties) * alloc.count, MEMORY_TAG_RENDERER);
    alloc.allocations = mem_allocate(sizeof(u32) * alloc.count, MEMORY_TAG_RENDERER);
    mem_memset(alloc.allocations, 0, sizeof(u32) * alloc.count);

    vkGetPhysicalDeviceQueueFamilyProperties(alloc.physical_device, &alloc.count, alloc.props);

    return alloc;
}

b8
_vc_db_queue_allocator_alloc(_vc_queue_allocator *alloc, VkQueueFlags required_flags, u32 *family_id, u32 *index)
{
    for(u32 i = 0; i < alloc->count; i++ )
    {
        if(
            ( (required_flags & alloc->props[i].queueFlags) == required_flags ) &&
            (alloc->allocations[i] < alloc->props[i].queueCount)
            )
        {
            if(family_id)
            {
                *family_id = i;
            }

            if(index)
            {
                *index = alloc->allocations[i];
            }

            alloc->allocations[i]++;

            return TRUE; // Queue was found and alloced
        }
    }

    return FALSE; // No available queue was found
}

void
_vc_db_queue_allocator_produce_ci(_vc_queue_allocator *alloc, VkDeviceQueueCreateInfo **queue_darray)
{
    for(u32 i = 0; i < alloc->count; i++)
    {
        if(alloc->allocations[i] == 0)
            continue;

        static const f32 zeros[32] =
        {
            0
        };
        VkDeviceQueueCreateInfo ci =
        {
            .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .pQueuePriorities = zeros,
            .queueCount       = alloc->allocations[i],
            .queueFamilyIndex = i,
        };
        darray_push(*queue_darray, ci);
    }
}

b8
_vc_db_queue_allocator_enable_present(_vc_queue_allocator *alloc, VkSurfaceKHR surface, u32 *family, u32 *index)
{
    u32 supporting_family = 0; // Index for found supporting family (everyone deserves one !!)

    // Check if a preallocated queue has present support
    b8 has_support = FALSE;
    for(u32 i = 0; i < alloc->count; i++)
    {
        VkBool32 support = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(alloc->physical_device, i, surface, &support);

        if(!support)
            continue;

        has_support = TRUE;

        // Is supported
        if(alloc->allocations[i] != 0)
        {
            // An already alloced queue was found with present support
            if(family)
                *family = i;

            if(index)
                *index = 0;

            return TRUE;
        }

        supporting_family = i;
    }

    if(!has_support)
    {
        return FALSE;
    }

    // If we end up here, support is existing, but not on previously allocated queues, allocate new queue
    if(family)
        *family = supporting_family;

    if(index)
        *index = 0;

    alloc->allocations[supporting_family]++; // Should be one
    return TRUE;
}

void
_vc_db_queue_allocator_destroy(_vc_queue_allocator   *alloc)
{
    mem_free(alloc->allocations);
    mem_free(alloc->props);
    *alloc = (_vc_queue_allocator)
    {
        0
    };
}

void
vc_device_wait_idle(vc_ctx   *ctx)
{
    vkDeviceWaitIdle(ctx->current_device);
}

void
vc_queue_wait_idle(vc_ctx *ctx, vc_queue queue)
{
    _vc_queue_intern *q = vc_handles_manager_deref(&ctx->handles_manager, queue);
    vkQueueWaitIdle(q->queue);
}

void
_vc_setup_vma(vc_ctx   *ctx)
{
    VmaVulkanFunctions vulkan_functions =
    {
        .vkGetInstanceProcAddr = &vkGetInstanceProcAddr,
        .vkGetDeviceProcAddr   = &vkGetDeviceProcAddr,
    };

    VmaAllocatorCreateInfo vma_ci =
    {
        .flags            = 0,
        .vulkanApiVersion = VK_API_VERSION_1_1,
        .physicalDevice   = ctx->current_physical_device,
        .device           = ctx->current_device,
        .instance         = ctx->vk_instance,
        .pVulkanFunctions = &vulkan_functions,
    };

    vmaCreateAllocator(&vma_ci, &ctx->main_allocator);
}

