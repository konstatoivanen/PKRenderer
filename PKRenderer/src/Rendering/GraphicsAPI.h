#pragma once
#include "Utilities/NoCopy.h"
#include "Rendering/Structs/Enums.h"
#include "Rendering/Objects/QueueSet.h"
#include "Utilities/Ref.h"
#include "Rendering/Objects/Buffer.h"
#include "Rendering/Objects/Texture.h"
#include "Rendering/Objects/BindArray.h"
#include "Rendering/Objects/AccelerationStructure.h"
#include "Rendering/BuiltInResources.h"

namespace PK::Rendering
{
    struct DriverMemoryInfo
    {
        uint32_t blockCount;
        uint32_t allocationCount;
        uint32_t unusedRangeCount;
        size_t usedBytes;
        size_t unusedBytes;
        size_t allocationSizeMin;
        size_t allocationSizeAvg;
        size_t allocationSizeMax;
        size_t unusedRangeSizeMin;
        size_t unusedRangeSizeAvg; 
        size_t unusedRangeSizeMax;
    };

    struct GraphicsDriver : public PK::Utilities::NoCopy
    {
        virtual ~GraphicsDriver() = default;
        virtual Structs::APIType GetAPI() const = 0;
        virtual Objects::QueueSet* GetQueues() const = 0;
        virtual DriverMemoryInfo GetMemoryInfo() const = 0;
        virtual std::string GetDriverHeader() const = 0;
        virtual size_t GetBufferOffsetAlignment(Structs::BufferUsage usage) const = 0;

        virtual void SetBuffer(uint32_t nameHashId, Objects::Buffer* buffer, const Structs::IndexRange& range) = 0;
        virtual void SetTexture(uint32_t nameHashId, Objects::Texture* texture, const Structs::TextureViewRange& range) = 0;
        virtual void SetBufferArray(uint32_t nameHashId, Objects::BindArray<Objects::Buffer>* bufferArray) = 0;
        virtual void SetTextureArray(uint32_t nameHashId, Objects::BindArray<Objects::Texture>* textureArray) = 0;
        virtual void SetImage(uint32_t nameHashId, Objects::Texture* texture, const Structs::TextureViewRange& range) = 0;
        virtual void SetSampler(uint32_t nameHashId, const Structs::SamplerDescriptor& sampler) = 0;
        virtual void SetAccelerationStructure(uint32_t nameHashId, Objects::AccelerationStructure* structure) = 0;
        virtual void SetConstant(uint32_t nameHashId, const void* data, uint32_t size) = 0;
        virtual void SetKeyword(uint32_t nameHashId, bool value) = 0;

        virtual void WaitForIdle() const = 0;
        virtual void GC() = 0;

        // @TODO this ownership model is bad. 
        // Should be part of raii but destructor execution order prevents that.
        inline void CreateBuiltInResources() { builtInResources = new BuiltInResources(); }
        inline void ReleaseBuiltInResources() { delete builtInResources; }

        static Utilities::Scope<GraphicsDriver> Create(const std::string& workingDirectory, Structs::APIType api);

        PK::Utilities::PropertyBlock globalResources = PK::Utilities::PropertyBlock(16384);
        BuiltInResources* builtInResources;
    };

    namespace GraphicsAPI
    {
        GraphicsDriver* GetActiveDriver();
        
        template<typename T>
        inline T* GetActiveDriver() 
        {
            static_assert(std::is_base_of<GraphicsDriver, T>::value, "Template argument type does not derive from GraphicsDriver!"); 
            return static_cast<T*>(GetActiveDriver()); 
        }

        Structs::APIType GetActiveAPI();
        PK::Rendering::Objects::QueueSet* GetQueues();
        DriverMemoryInfo GetMemoryInfo();
        size_t GetBufferOffsetAlignment(Structs::BufferUsage usage);
        const BuiltInResources* GetBuiltInResources();

        void SetBuffer(uint32_t nameHashId, PK::Rendering::Objects::Buffer* buffer, const Structs::IndexRange& range);
        void SetBuffer(uint32_t nameHashId, PK::Rendering::Objects::Buffer* buffer);
        void SetBuffer(const char* name, PK::Rendering::Objects::Buffer* buffer);
        void SetBuffer(const char* name, PK::Rendering::Objects::Buffer* buffer, const Structs::IndexRange& range);
        void SetTexture(uint32_t nameHashId, PK::Rendering::Objects::Texture* texture, const Structs::TextureViewRange& range);
        void SetTexture(uint32_t nameHashId, PK::Rendering::Objects::Texture* texture);
        void SetTexture(uint32_t nameHashId, PK::Rendering::Objects::Texture* texture, uint16_t level, uint16_t layer);
        void SetTexture(const char* name, PK::Rendering::Objects::Texture* texture);
        void SetTexture(const char* name, PK::Rendering::Objects::Texture* texture, uint16_t level, uint16_t layer);
        void SetTexture(const char* name, PK::Rendering::Objects::Texture* texture, const Structs::TextureViewRange& range);
        void SetImage(uint32_t nameHashId, PK::Rendering::Objects::Texture* texture, const Structs::TextureViewRange& range);
        void SetImage(uint32_t nameHashId, PK::Rendering::Objects::Texture* texture);
        void SetImage(uint32_t nameHashId, PK::Rendering::Objects::Texture* texture, uint16_t level, uint16_t layer);
        void SetImage(const char* name, PK::Rendering::Objects::Texture* texture);
        void SetImage(const char* name, PK::Rendering::Objects::Texture* texture, uint16_t level, uint16_t layer);
        void SetImage(const char* name, PK::Rendering::Objects::Texture* texture, const Structs::TextureViewRange& range);
        void SetSampler(uint32_t nameHashId, const PK::Rendering::Structs::SamplerDescriptor& sampler);
        void SetSampler(const char* name, const PK::Rendering::Structs::SamplerDescriptor& sampler);
        void SetAccelerationStructure(uint32_t nameHashId, PK::Rendering::Objects::AccelerationStructure* structure);
        void SetAccelerationStructure(const char* name, PK::Rendering::Objects::AccelerationStructure* structure);
        void SetBufferArray(uint32_t nameHashId, PK::Rendering::Objects::BindArray<PK::Rendering::Objects::Buffer>* bufferArray);
        void SetBufferArray(const char* name, PK::Rendering::Objects::BindArray<PK::Rendering::Objects::Buffer>* bufferArray);
        void SetTextureArray(uint32_t nameHashId, PK::Rendering::Objects::BindArray<PK::Rendering::Objects::Texture>* textureArray);
        void SetTextureArray(const char* name, PK::Rendering::Objects::BindArray<PK::Rendering::Objects::Texture>* textureArray);
        void SetConstant(uint32_t nameHashId, const void* data, uint32_t size);
        void SetConstant(const char* name, const void* data, uint32_t size);
        void SetKeyword(uint32_t nameHashId, bool value);
        void SetKeyword(const char* name, bool value);

        template<typename T>
        void SetConstant(uint32_t nameHashId, const T& value) { SetConstant(nameHashId, &value, (uint32_t)sizeof(T)); }

        template<typename T>
        void SetConstant(const char* name, const T& value) { SetConstant(name, &value, (uint32_t)sizeof(T)); }

        void GC();
    }
}