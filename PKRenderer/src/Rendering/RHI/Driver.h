#pragma once
#include "Utilities/NoCopy.h"
#include "Utilities/NativeInterface.h"
#include "Rendering/RHI/Structs.h"
#include "Rendering/RHI/Objects/QueueSet.h"
#include "Rendering/RHI/Objects/Buffer.h"
#include "Rendering/RHI/Objects/Texture.h"
#include "Rendering/RHI/Objects/BindArray.h"
#include "Rendering/RHI/Objects/AccelerationStructure.h"
#include "Rendering/RHI/BuiltInResources.h"

namespace PK::Rendering::RHI
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

    Utilities::Scope<struct Driver> CreateRHIDriver(const std::string& workingDirectory, APIType api);

    struct Driver : public PK::Utilities::NoCopy, public PK::Utilities::NativeInterface<Driver>
    {
        friend Utilities::Scope<Driver> PK::Rendering::RHI::CreateRHIDriver(const std::string& workingDirectory, APIType api);

        virtual ~Driver() = default;
        virtual APIType GetAPI() const = 0;
        virtual RHI::Objects::QueueSet* GetQueues() const = 0;
        virtual DriverMemoryInfo GetMemoryInfo() const = 0;
        virtual std::string GetDriverHeader() const = 0;
        virtual size_t GetBufferOffsetAlignment(BufferUsage usage) const = 0;

        virtual void SetBuffer(uint32_t nameHashId, RHI::Objects::Buffer* buffer, const IndexRange& range) = 0;
        virtual void SetTexture(uint32_t nameHashId, RHI::Objects::Texture* texture, const TextureViewRange& range) = 0;
        virtual void SetBufferArray(uint32_t nameHashId, RHI::Objects::BindArray<RHI::Objects::Buffer>* bufferArray) = 0;
        virtual void SetTextureArray(uint32_t nameHashId, RHI::Objects::BindArray<RHI::Objects::Texture>* textureArray) = 0;
        virtual void SetImage(uint32_t nameHashId, RHI::Objects::Texture* texture, const TextureViewRange& range) = 0;
        virtual void SetSampler(uint32_t nameHashId, const SamplerDescriptor& sampler) = 0;
        virtual void SetAccelerationStructure(uint32_t nameHashId, RHI::Objects::AccelerationStructure* structure) = 0;
        virtual void SetConstant(uint32_t nameHashId, const void* data, uint32_t size) = 0;
        virtual void SetKeyword(uint32_t nameHashId, bool value) = 0;

        void SetBuffer(uint32_t nameHashId, RHI::Objects::Buffer* buffer);
        void SetBuffer(const char* name, RHI::Objects::Buffer* buffer, const IndexRange& range);
        void SetBuffer(const char* name, RHI::Objects::Buffer* buffer);
        void SetTexture(uint32_t nameHashId, RHI::Objects::Texture* texture, uint16_t level, uint16_t layer);
        void SetTexture(uint32_t nameHashId, RHI::Objects::Texture* texture);
        void SetTexture(const char* name, RHI::Objects::Texture* texture, const TextureViewRange& range);
        void SetTexture(const char* name, RHI::Objects::Texture* texture, uint16_t level, uint16_t layer);
        void SetTexture(const char* name, RHI::Objects::Texture* texture);
        void SetBufferArray(const char* name, RHI::Objects::BindArray<RHI::Objects::Buffer>* bufferArray);
        void SetTextureArray(const char* name, RHI::Objects::BindArray<RHI::Objects::Texture>* textureArray);
        void SetImage(uint32_t nameHashId, RHI::Objects::Texture* texture, uint16_t level, uint16_t layer);
        void SetImage(const char* name, RHI::Objects::Texture* texture, const TextureViewRange& range);
        void SetImage(const char* name, RHI::Objects::Texture* texture, uint16_t level, uint16_t layer);
        void SetImage(uint32_t nameHashId, RHI::Objects::Texture* texture);
        void SetImage(const char* name, RHI::Objects::Texture* texture);
        void SetSampler(const char* name, const SamplerDescriptor& sampler);
        void SetAccelerationStructure(const char* name, RHI::Objects::AccelerationStructure* structure);
        void SetConstant(const char* name, const void* data, uint32_t size);
        void SetKeyword(const char* name, bool value);

        virtual void WaitForIdle() const = 0;
        virtual void GC() = 0;

        // @TODO this ownership model is bad. 
        // Should be part of raii but destructor execution order prevents that.
        inline void CreateBuiltInResources() { builtInResources = new BuiltInResources(); }
        inline void ReleaseBuiltInResources() { delete builtInResources; }
        
        static inline Driver* Get() { return s_instance; }

        template<typename Child>
        static inline Child* GetNative()
        {
            static_assert(std::is_base_of<Driver, Child>::value, "Child doesn't derive from base!");
            return static_cast<Child*>(s_instance);
        }

        PK::Utilities::PropertyBlock globalResources = PK::Utilities::PropertyBlock(16384);
        BuiltInResources* builtInResources;
        
        protected: inline static Driver* s_instance = nullptr;
    };
}