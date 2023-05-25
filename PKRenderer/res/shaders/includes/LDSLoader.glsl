#define DEFINE_LDS_LOADER(TStruct, TLoadFunc, TGroupSize, TFilterRadius, TName)                                                                                 \
                                                                                                                                                                \
    const int TName##_LDS_WIDTH = 2 * TFilterRadius + TGroupSize;                                                                                               \
    shared TStruct TName##_LDS_Data[TName##_LDS_WIDTH][TName##_LDS_WIDTH];                                                                                      \
                                                                                                                                                                \
    TStruct TName##_FetchLDSData(int xx, int yy)                                                                                                                \
    {                                                                                                                                                           \
        return TName##_LDS_Data[int(gl_LocalInvocationID.y) + TFilterRadius + yy][int(gl_LocalInvocationID.x) + TFilterRadius + xx];                            \
    }                                                                                                                                                           \
                                                                                                                                                                \
    void TName##_FillLDS(const int2 baseCoord, const int sharedOffset)                                                                                          \
    {                                                                                                                                                           \
        const int2 sharedCoord = int2(sharedOffset % TName##_LDS_WIDTH, sharedOffset / TName##_LDS_WIDTH);                                                      \
        TName##_LDS_Data[sharedCoord.y][sharedCoord.x] = TLoadFunc(baseCoord + sharedCoord);                                                                    \
    }                                                                                                                                                           \
                                                                                                                                                                \
    void TName##_PreloadLDS()                                                                                                                                   \
    {                                                                                                                                                           \
        const int2 globalBasePix = int2(gl_WorkGroupID.xy) * TGroupSize - int2(TFilterRadius);                                                                  \
        const int threadIndex = int(gl_LocalInvocationIndex);                                                                                                   \
        const int sharedCount = TName##_LDS_WIDTH * TName##_LDS_WIDTH;                                                                                          \
        const int threadCount = TGroupSize * TGroupSize;                                                                                                        \
        const int oneLoadCount = 2 * threadCount - sharedCount;                                                                                                 \
                                                                                                                                                                \
        if (threadIndex < oneLoadCount)                                                                                                                         \
        {                                                                                                                                                       \
            TName##_FillLDS(globalBasePix, threadIndex);                                                                                                        \
        }                                                                                                                                                       \
        else                                                                                                                                                    \
        {                                                                                                                                                       \
            const int neighborsIndex = threadIndex - oneLoadCount;                                                                                              \
            TName##_FillLDS(globalBasePix, oneLoadCount + neighborsIndex * 2 + 0);                                                                              \
            TName##_FillLDS(globalBasePix, oneLoadCount + neighborsIndex * 2 + 1);                                                                              \
        }                                                                                                                                                       \
    }                                                                                                                                                           \
                                                                                                                                                                \