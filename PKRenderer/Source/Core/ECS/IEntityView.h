#pragma once
#include "Core/Utilities/Templates.h"
#include "Core/ECS/EntityComponentRef.h"
#include "Core/ECS/EGID.h"

namespace PK
{
    struct IEntityView
    {
        EGID GID;
    };
}

/*
    Macro for creating an entity view that supports 
    - auto assigning components
    - @TODO gui
    - @TODO serialization.
*/

#define PK_ECS_VIEW_BEGIN(TView) \
    struct TView : public PK::IEntityView \
    { \
        TView() {}; \
    private: \
        typedef TView meta_TView; \
        struct meta_FirstId {}; \
        template<typename TImpl> inline static void* meta_AssignPrev(meta_FirstId, [[maybe_unused]] meta_TView* view, TImpl* impl) \
        { \
            return nullptr; \
        } \
        typedef meta_FirstId \

#define PK_ECS_VIEW_COMPONENT(TType, TName) \
        meta_Id##TName; \
    public: EntityComponentRef<TType> TName; \
    private: \
        struct meta_NextId##TName {}; \
        template<typename TImpl> inline static void meta_AssignPrev(meta_NextId##TName, meta_TView* view, TImpl* impl) \
        { \
            if constexpr (TIsConvertible<TImpl*, TType*>) view->TName = impl; \
            meta_AssignPrev(meta_Id##TName(), view, impl); \
        } \
        typedef meta_NextId##TName \

#define PK_ECS_VIEW_END() \
        meta_LastId; \
    public: \
        template<typename TImpl> inline void SetImplementer(TImpl* impl) \
        { \
            meta_AssignPrev<TImpl>(meta_LastId(), this, impl); \
        } \
    }; \
