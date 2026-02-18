#pragma once
#include <cstdint>

namespace PK
{
    // Modify these if running into limits or resizes.

    constexpr static const uint64_t PK_VK_FRAME_ARENA_SIZE = 16384ull;

    constexpr static const uint64_t PK_VK_GLOBAL_PROPERTIES_INITIAL_SIZE = 16384ull;
    constexpr static const uint64_t PK_VK_GLOBAL_PROPERTIES_INITIAL_COUNT = 128ull;

    constexpr static const uint64_t PK_VK_QUEUE_SEMAPHORE_COUNT = 16ull;
 
    constexpr static const uint64_t PK_VK_MAX_BUFFER_VIEWS = 1024ull;
    constexpr static const uint64_t PK_VK_MAX_IMAGE_VIEWS = 1024ull;
    constexpr static const uint64_t PK_VK_MAX_RAW_IMAGES = 1024ull;
    constexpr static const uint64_t PK_VK_MAX_RAW_BUFFERS = 1024ull;
    constexpr static const uint64_t PK_VK_MAX_ACCELERATION_STRUCTURES = 1024ull;

    constexpr static const uint64_t PK_VK_DESCRIPTOR_INITIAL_COUNT_SETS = 256ull;
    constexpr static const uint64_t PK_VK_DESCRIPTOR_INITIAL_COUNT_SAMPLERS = 32ull;
    constexpr static const uint64_t PK_VK_DESCRIPTOR_INITIAL_COUNT_STORAGE_BUFFER = 128ull;
    constexpr static const uint64_t PK_VK_DESCRIPTOR_INITIAL_COUNT_UNIFORM_BUFFER = 128ull;
    constexpr static const uint64_t PK_VK_DESCRIPTOR_INITIAL_COUNT_SAMPLED_IMAGE = 128ull;
    constexpr static const uint64_t PK_VK_DESCRIPTOR_INITIAL_COUNT_STORAGE_IMAGE = 128ull;
    constexpr static const uint64_t PK_VK_DESCRIPTOR_INITIAL_COUNT_COMBINED_IMAGE = 128ull;
    constexpr static const uint64_t PK_VK_DESCRIPTOR_INITIAL_COUNT_ACCELERATION_STRUCTURE = 16ull;

    constexpr static const uint64_t PK_VK_MAX_DESCRIPTOR_TYPE_POOL_SIZES = 32ull;
    constexpr static const uint64_t PK_VK_MAX_DESCRIPTOR_SETS = 1024ull;
    constexpr static const uint64_t PK_VK_MAX_DESCRIPTOR_BINDINGS = 4096ull;
    constexpr static const uint64_t PK_VK_MAX_DESCRIPTOR_SET_POOLS = 8ull;
    constexpr static const uint64_t PK_VK_DESCRIPTOR_WRITE_ARENA_SIZE = 8192ull;

    constexpr static const uint64_t PK_VK_MAX_SPARSE_PAGES = 1024ull;
    constexpr static const uint64_t PK_VK_MAX_SPARSE_RANGES = 1024ull;
    
    constexpr static const uint64_t PK_VK_MAX_STAGING_BUFFERS = 512ull;
    
    constexpr static const uint64_t PK_VK_MAX_SAMPLERS = 32ull;
    
    constexpr static const uint64_t PK_VK_MAX_PIPELINES = 2048ull;
    constexpr static const uint64_t PK_VK_MAX_PIPELINES_VERTEX = 1024ull;
    constexpr static const uint64_t PK_VK_MAX_PIPELINES_MESH = 1024ull;
    constexpr static const uint64_t PK_VK_MAX_PIPELINES_GENERIC = 1024ull;

    constexpr static const uint64_t PK_VK_MAX_DESCRIPTOR_SET_LAYOUTS = 1024ull;
    constexpr static const uint64_t PK_VK_MAX_PIPELINE_LAYOUTS = 1024ull;

    constexpr static const uint64_t PK_VK_MAX_COMMAND_BUFFERS = 24ull;

    constexpr static const uint64_t PK_VK_MAX_ACCESS_HEADERS = 1024ull;
    constexpr static const uint64_t PK_VK_MAX_ACCESS_RECORDS = 1024ull;
    constexpr static const uint64_t PK_VK_MAX_BUFFER_BARRIERS = 256ull;
    constexpr static const uint64_t PK_VK_MAX_IMAGE_BARRIERS = 256ull;
}
