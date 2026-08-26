#pragma once
#include "Core/Base/Containers/HashMap.h"
#include "Core/ECS/EntityComposition.h"
#include "Core/ECS/EntityComponentMeta.h"
#include "Core/ECS/EntityVisitor.h"
#include "Core/ECS/EntityID.h"

namespace PK
{
    struct EntityDatabase
    {
        constexpr const static uint64_t VIEW_MASK = 0x8000000000000000ull;
        constexpr const static uint64_t COMP_MASK = 0x7FFFFFFFFFFFFFFFull;

        struct Composition
        {
            const EntityComponentMeta* components;
            const EntityVisitor* visitors;
            const uint32_t componentStride;
            const uint16_t componentCount;
            const uint16_t visitorCount;
            uint32_t capacity;
            uint32_t count;
            void* buffer;

            void** GetStreams() { return static_cast<void**>(buffer); }
        };

        template<typename TView>
        struct ViewIterator
        {
            struct Sentinel {};
            EntityDatabase* entityDb;
            Composition* viewdata;
            TView view;

            uint32_t groupIndex = 0u;
            uint32_t arrayCount = 0u;
            bool isValid = false;

            constexpr ViewIterator(EntityDatabase* db, Composition* data) noexcept : entityDb(db), viewdata(data), isValid(Next()) {}
            TView& operator*() { return view; }
            TView* operator->() { return &view; }
            const TView& operator*() const { return view; }
            const TView* operator->() const { return &view; }
            ViewIterator& operator++() { isValid = Next(); return *this; }
            void operator++(int) { ++(*this); }
            friend bool operator!=(const ViewIterator& it, Sentinel) noexcept { return it.isValid; }

            bool Next()
            {
                if (arrayCount)
                {
                    ReflectFields(view, [](auto& field)
                    {
                        if constexpr (TIsPointer<TRemoveCVRef_T<decltype(field)>>)
                        {
                            field++;
                        }
                    });
                    arrayCount--;
                    return true;
                }

                while (groupIndex < viewdata->count)
                {
                    auto index = static_cast<const uint32_t*>(viewdata->buffer)[groupIndex++];
                    auto comp = &entityDb->m_compositions[index].value;
                    
                    if (comp->count)
                    {
                        arrayCount = comp->count - 1ull;
                        view = entityDb->BindView<TView>(index, 0);
                        return true;
                    }
                }

                return false;
            }
        };

        template<typename TView>
        struct ViewRange
        {
            EntityDatabase* entityDb;
            Composition* viewdata;

            size_t count() const
            {
                size_t count = 0u;

                for (auto i = 0u; i < viewdata->count; ++i)
                {
                    auto index = static_cast<const uint32_t*>(viewdata->buffer)[i];
                    auto comp = &entityDb->m_compositions[index].value;
                    count += comp->count;
                }

                return count;
            }

            auto begin() const { return ViewIterator<TView>(entityDb, viewdata); }
            auto end() const { return typename ViewIterator<TView>::Sentinel{}; }
        };

        EntityDatabase(uint32_t compositionCapacity, uint32_t entityCapacity);
        ~EntityDatabase();

        template<typename TEntityStruct>
        void Reserve(size_t entryCount)
        {
            AllocateComposition<TEntityStruct>(false, entryCount);
        }

        template<typename TEntityStruct> 
        TEntityStruct New()
        {
            auto compositionIndex = AllocateComposition<TEntityStruct>(false, 1ull);
            auto identifier = NewEntity(compositionIndex);
            return BindView<TEntityStruct>(compositionIndex, identifier.arrayIndex());
        }

        template<typename TEntityStruct, typename TDescriptor>
        requires TEntityHasOnCreate<TEntityStruct, TDescriptor>
        TEntityStruct New(const TDescriptor& descriptor)
        {
            auto entity = New<TEntityStruct>();
            TEntityStruct::OnCreate(this, entity, descriptor);
            return entity;
        }

        template<typename TViewStruct>
        ViewRange<TViewStruct> Query()
        {
            auto viewIndex = AllocateComposition<TViewStruct>(true, 0ull);
            return { this, &m_compositions[viewIndex].value };
        }

        template<typename TViewStruct>
        TViewStruct Query(uint32_t entityId)
        {
            auto* id = m_identifiers.GetValuePtr(EntityID(entityId, 0u, 0u));
            return id ? BindView<TViewStruct>(id->compositionIndex(), id->arrayIndex()) : TViewStruct{};
        }

        template<typename TVisitorData>
        void Visit(uint32_t entityId, TVisitorData* userdata)
        {
            VisitEntity(entityId, pk_type_uuid64<TVisitorData>, userdata);
        }

        template<typename TVisitorData>
        uint32_t VisitType(uint64_t compositionUUID, TVisitorData* userdata)
        {
            uint32_t entityId = 0u;
            VisitComposition(compositionUUID, pk_type_uuid64<TVisitorData>, userdata, &entityId);
            return entityId;
        }

        template<typename TEntityStruct>
        void DeleteType() 
        {
            static_assert(TIsValidEntityStruct<TEntityStruct>, "Struct type is not a valid entity composition!");
            using TComposition = TStructToEntityComposition<TEntityStruct>;
            const auto compositionUUID = pk_type_uuid64<TComposition>;
            DeleteType(compositionUUID);
        }

        void Delete(uint32_t entityId);

    private:
        template<typename TStruct>
        uint32_t AllocateComposition(bool is_view, size_t newEntryCount)
        {
            static_assert(TIsValidEntityStruct<TStruct>, "Struct type is not a valid entity composition!");

            using TComposition = TStructToEntityComposition<TStruct>;
            const auto compositionUUID = pk_type_uuid64<TComposition>;
            const auto typeKey = (compositionUUID & COMP_MASK) | (uint64_t(is_view) << 63ull);
            const auto index = m_compositions.AddKey(typeKey);
            const auto isNew = m_compositions[index].value.componentCount == 0ull;

            if (isNew)
            {
                const auto visitors = GetEntityVisitors<TStruct>();

                Memory::Construct(&m_compositions[index].value, 
                    entity_component_metas<TComposition>.data,
                    visitors.visitors,
                    static_cast<uint32_t>(TComposition::Stride),
                    static_cast<uint16_t>(TComposition::Size),
                    static_cast<uint16_t>(visitors.count),
                    0u, 0u, nullptr);
            }
            
            if (isNew && is_view)
            {
                UpdateViewIndices(index);
            }

            if (!is_view)
            {
                ReserveEntitities(index, newEntryCount);
            }

            return index;
        }

        template<typename TView>
        TView BindView(uint32_t compositionIndex, uint32_t arrayOffset)
        {
            static_assert(TIsValidEntityStruct<TView>, "Struct type is not a valid entity composition!");

            auto* comp = &m_compositions[compositionIndex].value;
            TView view{};

            ReflectFields(view, [comp, arrayOffset](auto& field)
            {
                using TField = TRemoveCVRef_T<decltype(field)>;

                if constexpr (TIsPointer<TField>)
                {
                    constexpr const auto uuid = pk_type_uuid64<TRemovePtr_T<TField>>;

                    for (auto i = 0u; i < comp->componentCount; ++i)
                    {
                        if (comp->components[i].typeUUID == uuid)
                        {
                            field = static_cast<TField>(comp->GetStreams()[i]) + arrayOffset;
                            break;
                        }
                    }
                }
            });

            return view;
        }

        EntityID NewEntity(uint32_t compositionIndex);
        void DeleteType(uint64_t compsotionUUID);

        uint32_t* GetEntityIdStream(uint32_t compositionIndex);
        void VisitEntity(uint32_t entityId, uint64_t visitorUUID, void* userdata);
        void VisitComposition(uint64_t compositionUUID, uint64_t visitorUUID, void* userdata, uint32_t* entityId);
        void ReserveEntitities(uint32_t compositionIndex, size_t entryCount);
        void UpdateViewIndices(uint32_t viewIndex);

        HashSet<EntityID, EntityIDHash> m_identifiers;
        HashMap<uint64_t, Composition> m_compositions;
        uint32_t m_idCounter = 0u;
    };
}
