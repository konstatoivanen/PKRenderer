#pragma once
#include <stdint.h>

namespace PK
{
    constexpr static const uint64_t PK_VK_GLOBAL_PROPERTIES_INITIAL_SIZE = 16384ull;
    constexpr static const uint64_t PK_VK_GLOBAL_PROPERTIES_INITIAL_COUNT = 128ull;

    constexpr static const uint64_t PK_VK_MAX_BUFFERS = 1024ull;
    constexpr static const uint64_t PK_VK_MAX_IMAGES = 1024ull;
    constexpr static const uint64_t PK_VK_MAX_SHADERS = 2048ull;

    constexpr static const uint64_t PK_VK_MAX_SAMPLERS = 32ull;
    constexpr static const uint64_t PK_VK_MAX_DESCRIPTORS = 4096ull;

    constexpr static const uint32_t PK_VK_MAX_RENDER_TARGET_VIEWS = 256u;
    constexpr static const uint32_t PK_VK_MAX_DESCRIPTOR_RANGES = 8u;

    constexpr static const uint32_t PK_VK_MAX_RESOURCE_DESCRIPTOR_SIZE = 64u;
    constexpr static const uint32_t PK_VK_MAX_SAMPLER_DESCRIPTOR_SIZE = 32u;
    constexpr static const uint32_t PK_VK_PUSH_OFFSET_RESOURCE_HEAP = 248u;
    constexpr static const uint32_t PK_VK_PUSH_OFFSET_SAMPLER_HEAP = 252u;
    
    constexpr static const uint64_t PK_VK_MAX_PIPELINES = 2048ull;
    constexpr static const uint64_t PK_VK_MAX_PIPELINES_VERTEX = 1024ull;
    constexpr static const uint64_t PK_VK_MAX_PIPELINES_MESH = 1024ull;
    constexpr static const uint64_t PK_VK_MAX_PIPELINES_GENERIC = 1024ull;

    constexpr static const uint64_t PK_VK_MAX_COMMAND_BUFFERS = 24ull;

    constexpr static const uint64_t PK_VK_MAX_ACCESS_RESOURCES = 8192ull;
    constexpr static const uint64_t PK_VK_MAX_ACCESS_NODES = 8192ull;
    constexpr static const uint64_t PK_VK_MAX_ACCESS_PASSES = 512ull;
    constexpr static const uint64_t PK_VK_MAX_ACCESS_IMAGE_BARRIERS = 2048ull;

    constexpr static const uint32_t PK_VK_MAX_AS_COMPACTIONS = 512u;

    constexpr const static uint32_t PK_VK_MAX_GRAPH_SCOPES = 4u;

    constexpr const static uint32_t PK_VK_MAX_TIMERS = 4096u;
    constexpr const static uint32_t PK_VK_MAX_TIMER_QUERIES = PK_VK_MAX_TIMERS * 2ull;
    constexpr const static uint32_t PK_VK_MAX_TIMER_SCOPES = 64u;
    constexpr const static uint32_t PK_VK_MAX_TIMER_STACK = 32u;
    constexpr const static uint32_t PK_VK_MAX_DEBUG_STACK = 32u;

}
