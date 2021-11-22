#pragma once
#include "PrecompiledHeader.h"
#include "Core/IService.h"
#include "Core/BufferView.h"
#include "Math/PKMath.h"
#include "Utilities/Ref.h"

namespace PK::ECS
{
    using namespace PK::Math;
    using namespace PK::Utilities;
    using namespace PK::Core;

    enum class ENTITY_GROUPS
    {
        INVALID = 0,
        INACTIVE = 1,
        ACTIVE = 2,
        FREE = 3
    };

    struct EGID
    {
        public:
            inline uint entityID() const { return (uint)(_GID & 0xFFFFFFFF); }
            inline uint groupID() const { return (uint)(_GID >> 32); }
            EGID() : _GID(0) {}
            EGID(const EGID& other) : _GID(other._GID) {}
            EGID(ulong identifier) : _GID(identifier) {}
            EGID(uint entityID, uint groupID) : _GID((ulong)groupID << 32 | ((ulong)(uint)entityID & 0xFFFFFFFF)) {}
            inline bool IsValid() const { return _GID > 0; }
            
            inline bool operator ==(const EGID& obj2) const { return _GID == obj2._GID; }
            inline bool operator !=(const EGID& obj2) const { return _GID != obj2._GID; }
            inline bool operator <(const EGID& obj2) const { return _GID < obj2._GID; }
            inline bool operator >(const EGID& obj2) const { return _GID > obj2._GID; }

        private:
            ulong _GID;
    };

    const EGID EGIDDefault = EGID(1);
    const EGID EGIDInvalid = EGID(0);

    struct IImplementer
    {
        virtual ~IImplementer() = default;
    };

    struct IEntityView
    {
        EGID GID;
        virtual ~IEntityView() = default;
    };
    
    const uint PK_ECS_BUCKET_SIZE = 32000;

    struct ImplementerBucket
    {
        void (*destructor)(void* value);
        void* data;

        ~ImplementerBucket() { destructor(data); }
    };

    struct ImplementerContainer
    {
        size_t count = 0;
        std::vector<Scope<ImplementerBucket>> buckets;
    };

    struct EntityViewsCollection
    {
        std::map<uint, size_t> Indices;
        std::vector<char> Buffer;
    };

    struct ViewCollectionKey
    {
        std::type_index type;
        uint group;
        
        inline bool operator < (const ViewCollectionKey& r) const noexcept
        {
            return (type < r.type) || ((type == r.type) && (group < r.group));
        }
    };


    class EntityDatabase : public IService
    {
        public:
            inline int ReserveEntityId() { return ++m_idCounter; }

            template<typename T>
            T* ResereveImplementer()
            {
                auto type = std::type_index(typeid(T));
                auto& container = m_implementerBuckets[type];

                size_t elementsPerBucket = PK_ECS_BUCKET_SIZE / sizeof(T);
                size_t bucketIndex = container.count / elementsPerBucket;
                size_t subIndex = container.count - bucketIndex * elementsPerBucket;
                ++container.count;

                if (container.buckets.size() <= bucketIndex)
                {
                    auto newBucket = new ImplementerBucket();
                    newBucket->data = new T[elementsPerBucket];
                    newBucket->destructor = [](void* v) { delete[] reinterpret_cast<T*>(v); };
                    container.buckets.push_back(Scope<ImplementerBucket>(newBucket));
                }

                return reinterpret_cast<T*>(container.buckets.at(bucketIndex).get()->data) + subIndex;
            }

            template<typename T>
            T* ReserveEntityView(const EGID& egid)
            {
                PK_THROW_ASSERT(egid.IsValid(), "Trying to acquire resources for an invalid egid!");
                auto& views = m_entityViews[{ std::type_index(typeid(T)), egid.groupID() }];
                auto offset = views.Buffer.size();
                views.Buffer.resize(offset + sizeof(T));
                views.Indices[egid.entityID()] = offset;

                auto* element = reinterpret_cast<T*>(views.Buffer.data() + offset);

                element->GID = egid;
                
                return element;
            }

            template<typename T>
            const BufferView<T> Query(const uint group)
            {
                PK_THROW_ASSERT(group, "Trying to acquire resources for an invalid egid!");
                auto& views = m_entityViews[{ std::type_index(typeid(T)), group }];
                auto count = views.Buffer.size() / sizeof(T);
                return { reinterpret_cast<T*>(views.Buffer.data()), count };
            }

            template<typename T>
            T* Query(const EGID& egid)
            {
                PK_THROW_ASSERT(egid.IsValid(), "Trying to acquire resources for an invalid egid!");
                auto& views = m_entityViews.at({ std::type_index(typeid(T)), egid.groupID() });
                auto offset = views.Indices.at(egid.entityID());
                return reinterpret_cast<T*>(views.Buffer.data() + offset);
            }

        private:
            std::map<ViewCollectionKey, EntityViewsCollection> m_entityViews;
            std::map<std::type_index, ImplementerContainer> m_implementerBuckets;
            int m_idCounter = 0;
    };
}