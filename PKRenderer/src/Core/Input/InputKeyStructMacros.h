#pragma once
#include "Core/Input/InputKeyConfig.h"

#define PK_INPUTKEY_STRUCT_BEGIN(TType) \
    struct TType \
    { \
        TType() {}; \
    private: \
        typedef PK::InputKeyConfig meta_TConfig; \
        typedef TType meta_ThisType; \
        struct meta_FirstId {}; \
        typedef void*(*meta_ParseFunc)(meta_FirstId, meta_ThisType*, const meta_TConfig* config); \
        static void* meta_ParsePrev(meta_FirstId, meta_ThisType* thisPtr, const meta_TConfig* config) \
        { \
        return nullptr; \
        } \
        typedef meta_FirstId \

#define PK_INPUTKEY_STRUCT_MEMBER(TNameSpace, TName, TInitialValue) \
        meta_Id##TName; \
    public: PK::InputKey TName = PK::InputKey::TInitialValue; \
    private: \
        struct meta_NextId##TName {}; \
        static void* meta_ParsePrev(meta_NextId##TName, meta_ThisType* thisPtr, const meta_TConfig* config) \
        { \
            config->CommandInputKeys.TryGetKey(#TNameSpace "." #TName, &thisPtr->TName); \
            void*(*PrevFunc)(meta_Id##TName, meta_ThisType*, const meta_TConfig*); \
            PrevFunc = meta_ParsePrev; \
            return (void*)PrevFunc; \
        } \
        typedef meta_NextId##TName \

#define PK_INPUTKEY_STRUCT_END() \
        meta_LastId; \
    public: \
        inline void SetKeysFrom(const meta_TConfig* config) \
        { \
            void*(*lastParseFunc)(meta_LastId, meta_ThisType*, const meta_TConfig*); \
            lastParseFunc = meta_ParsePrev; \
            void* parseFunc = (void*)lastParseFunc; \
            do \
            { \
                parseFunc = reinterpret_cast<meta_ParseFunc>(parseFunc)(meta_FirstId(), this, config); \
            } \
            while (parseFunc); \
        } \
    }; \
