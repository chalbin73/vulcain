#pragma once

enum memory_alloc_tag
{
    // Fixed tags, they should stay here as they are necessary to the base layer
    MEMORY_TAG_UNKNOWN,
    MEMORY_TAG_DARRAY,
    MEMORY_TAG_STRING,
    MEMORY_TAG_FIO_DATA, // Allocated data used to store File inputs temporarly in memory (read_file)

    // Free tags, those can be anything
    MEMORY_TAG_RENDERER,    // Allocated data used by the renderer for it to work
    MEMORY_TAG_RENDER_DATA, // Allocated data exploited by the renderer (Voxel data for example, textures ...)

    // Tag count
    MEMORY_TAG_COUNT,
};

__attribute__( (unused) ) static char *memory_alloc_tag_names[MEMORY_TAG_COUNT] =
{
    // Fixed tags, they should stay here as they are necessary to the base layer
    "MEMORY_UNKNOWN    ",
    "MEMORY_DARRAY     ",
    "MEMORY_STRING     ",
    "MEMORY_FIO_DATA   ",

    // Free tags, those can be anything
    "MEMORY_RENDERER   ",
    "MEMORY_RENDER_DATA",
};

