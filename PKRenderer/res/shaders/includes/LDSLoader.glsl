#pragma once

#define DEFINE_LDS_LOADER(TLoadFunc, TLoadParams, TName)                                                        \
                                                                                                                \
    void LoadLDS_##TName(TLoadParams loadParams, const int sharedCount)                                         \
    {                                                                                                           \
        const int threadIndex = int(gl_LocalInvocationIndex);                                                   \
                                                                                                                \
        if (threadIndex >= sharedCount)                                                                         \
        {                                                                                                       \
            return;                                                                                             \
        }                                                                                                       \
                                                                                                                \
        const int totalThreadCount = int(gl_WorkGroupSize.x * gl_WorkGroupSize.y * gl_WorkGroupSize.z);         \
        const int threadCount = min(sharedCount, totalThreadCount);                                             \
        const int loadsPerThread = sharedCount / threadCount;                                                   \
        const int oneLoadCount = sharedCount - loadsPerThread * threadCount;                                    \
                                                                                                                \
        for (int i = 0; i < loadsPerThread; ++i)                                                                \
        {                                                                                                       \
            TLoadFunc(loadParams, threadIndex * loadsPerThread + i);                                            \
        }                                                                                                       \
                                                                                                                \
        if (threadIndex < oneLoadCount)                                                                         \
        {                                                                                                       \
            TLoadFunc(loadParams, threadCount * loadsPerThread + threadIndex);                                  \
        }                                                                                                       \
    }                                                                                                           \
                                                                                                                \

#define DEFINE_LDS_LOADER_SIMPLE(TLoadFunc, TName)                                                              \
                                                                                                                \
    void LoadLDS_##TName(const int sharedCount)                                                                 \
    {                                                                                                           \
        const int threadIndex = int(gl_LocalInvocationIndex);                                                   \
                                                                                                                \
        if (threadIndex >= sharedCount)                                                                         \
        {                                                                                                       \
            return;                                                                                             \
        }                                                                                                       \
                                                                                                                \
        const int totalThreadCount = int(gl_WorkGroupSize.x * gl_WorkGroupSize.y * gl_WorkGroupSize.z);         \
        const int threadCount = min(sharedCount, totalThreadCount);                                             \
        const int loadsPerThread = sharedCount / threadCount;                                                   \
        const int oneLoadCount = sharedCount - loadsPerThread * threadCount;                                    \
                                                                                                                \
        for (int i = 0; i < loadsPerThread; ++i)                                                                \
        {                                                                                                       \
            TLoadFunc(threadIndex * loadsPerThread + i);                                                        \
        }                                                                                                       \
                                                                                                                \
        if (threadIndex < oneLoadCount)                                                                         \
        {                                                                                                       \
            TLoadFunc(threadCount * loadsPerThread + threadIndex);                                              \
        }                                                                                                       \
    }                                                                                                           \
                                                                                                                \
