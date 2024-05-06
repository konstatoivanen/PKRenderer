#pragma once
#include "Utilities/BufferView.h"
#include "Utilities/Ref.h"
#include "Core/IService.h"
#include "ECS/EGID.h"

namespace PK::ECS
{
    enum class ENTITY_GROUPS
    {
        INVALID = 0,
        INACTIVE = 1,
        ACTIVE = 2,
        FREE = 3
    };

    struct IImplementer
    {
        virtual ~IImplementer() = default;
    };

    struct IEntityView
    {
        EGID GID;
        virtual ~IEntityView() = default;
    };

    constexpr static const uint32_t PK_ECS_BUCKET_SIZE = 32000;

    struct ImplementerBucket
    {
        void (*destructor)(void* value);
        void* data;

        ~ImplementerBucket() { destructor(data); }
    };

    struct ImplementerContainer
    {
        size_t count = 0;
        std::vector<Utilities::Scope<ImplementerBucket>> buckets;
    };

    struct EntityViewsCollection
    {
        std::unordered_map<uint32_t, size_t> indices;
        std::vector<uint8_t> buffer;
    };

    struct ViewCollectionKey
    {
        std::type_index type;
        uint32_t group;

        inline bool operator == (const ViewCollectionKey& r) const noexcept
        {
            return type == r.type && group == r.group;
        }
    };

    struct ViewCollectionKeyHash 
    {
        std::size_t operator()(const ViewCollectionKey& k) const
        {
            return k.type.hash_code() ^ k.group;
        }
    };

    class EntityDatabase : public Core::IService
    {
    public:
        constexpr uint32_t ReserveEntityId() { return ++m_idCounter; }
        inline EGID ReserveEntityId(uint32_t groupId) { return EGID(ReserveEntityId(), groupId); }

        template<typename T>
        T* ReserveImplementer()
        {
            static_assert(std::is_base_of<IImplementer, T>::value, "Template argument type does not derive from IImplementer!");

            auto type = std::type_index(typeid(T));
            auto& container = m_implementerBuckets[type];

            uint64_t elementsPerBucket = PK_ECS_BUCKET_SIZE / sizeof(T);
            uint64_t bucketIndex = container.count / elementsPerBucket;
            uint64_t subIndex = container.count - bucketIndex * elementsPerBucket;
            ++container.count;

            if (container.buckets.size() <= bucketIndex)
            {
                auto newBucket = new ImplementerBucket();
                newBucket->data = new T[elementsPerBucket];
                newBucket->destructor = [](void* v) { delete[] reinterpret_cast<T*>(v); };
                container.buckets.push_back(Utilities::Scope<ImplementerBucket>(newBucket));
            }

            return reinterpret_cast<T*>(container.buckets.at(bucketIndex).get()->data) + subIndex;
        }

        template<typename TView>
        TView* ReserveView(const EGID& egid)
        {
            static_assert(std::is_base_of<IEntityView, TView>::value, "Template argument type does not derive from IEntityView!");

            if (!egid.IsValid())
            {
                throw std::runtime_error("Trying to acquire resources for an invalid egid!");
            }

            auto& views = m_entityViews[{ std::type_index(typeid(TView)), egid.groupID() }];
            auto offset = views.buffer.size();
            views.buffer.resize(offset + sizeof(TView));
            views.indices[egid.entityID()] = offset;
            auto* element = reinterpret_cast<TView*>(views.buffer.data() + offset);
            element->GID = egid;
            return element;
        }

        template<typename TImpl, typename TView, typename ...M>
        TView* ReserveView(TImpl* implementer, const EGID& egid, M TView::* ...params)
        {
            auto* view = ReserveView<TView>(egid);
            static_assert((... && std::is_assignable<decltype(view->*params), TImpl*>::value), "Components are not present in implementer");
            ((view->*params = static_cast<M>(implementer)), ...);
            return view;
        }

        template<typename TView>
        const Utilities::BufferView<TView> Query(const uint32_t group)
        {
            static_assert(std::is_base_of<IEntityView, TView>::value, "Template argument type does not derive from IEntityView!");

            if (!group)
            {
                throw std::runtime_error("Trying to acquire resources for an invalid egid!");
            }

            auto& views = m_entityViews[{ std::type_index(typeid(TView)), group }];
            auto count = views.buffer.size() / sizeof(TView);
            return { reinterpret_cast<TView*>(views.buffer.data()), count };
        }

        template<typename TView>
        TView* Query(const EGID& egid)
        {
            static_assert(std::is_base_of<IEntityView, TView>::value, "Template argument type does not derive from IEntityView!");

            if (!egid.IsValid())
            {
                throw std::runtime_error("Trying to acquire resources for an invalid egid!");
            }

            auto& views = m_entityViews.at({ std::type_index(typeid(TView)), egid.groupID() });
            auto offset = views.indices.at(egid.entityID());
            return reinterpret_cast<TView*>(views.buffer.data() + offset);
        }

    private:
        std::unordered_map<ViewCollectionKey, EntityViewsCollection, ViewCollectionKeyHash> m_entityViews;
        std::unordered_map<std::type_index, ImplementerContainer> m_implementerBuckets;
        uint32_t m_idCounter = 0;
    };
}

