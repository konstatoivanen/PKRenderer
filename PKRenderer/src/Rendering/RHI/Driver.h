#pragma once
#include <string>
#include "Utilities/NoCopy.h"
#include "Utilities/NativeInterface.h"
#include "Utilities/PropertyBlock.h"
#include "Rendering/RHI/RHI.h"

namespace PK::Rendering::RHI
{
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
        
        virtual RHI::Objects::AccelerationStructureRef CreateAccelerationStructure(const char* name) = 0;
        virtual RHI::Objects::TextureBindArrayRef CreateTextureBindArray(size_t capacity) = 0;
        virtual RHI::Objects::BufferBindArrayRef CreateBufferBindArray(size_t capacity) = 0;
        virtual RHI::Objects::BufferRef CreateBuffer(size_t size, BufferUsage usage, const char* name) = 0;
        virtual RHI::Objects::TextureRef CreateTexture(const TextureDescriptor& descriptor, const char* name) = 0;
        virtual RHI::Objects::WindowScope CreateRHIWindow(const WindowProperties& properties) = 0;

        virtual void SetBuffer(Utilities::NameID name, RHI::Objects::Buffer* buffer, const IndexRange& range) = 0;
        virtual void SetTexture(Utilities::NameID name, RHI::Objects::Texture* texture, const TextureViewRange& range) = 0;
        virtual void SetBufferArray(Utilities::NameID name, RHI::Objects::BufferBindArray* bufferArray) = 0;
        virtual void SetTextureArray(Utilities::NameID name, RHI::Objects::TextureBindArray* textureArray) = 0;
        virtual void SetImage(Utilities::NameID name, RHI::Objects::Texture* texture, const TextureViewRange& range) = 0;
        virtual void SetSampler(Utilities::NameID name, const SamplerDescriptor& sampler) = 0;
        virtual void SetAccelerationStructure(Utilities::NameID name, RHI::Objects::AccelerationStructure* structure) = 0;
        virtual void SetConstant(Utilities::NameID name, const void* data, uint32_t size) = 0;
        virtual void SetKeyword(Utilities::NameID name, bool value) = 0;

        void SetBuffer(Utilities::NameID name, RHI::Objects::Buffer* buffer);
        void SetTexture(Utilities::NameID name, RHI::Objects::Texture* texture, uint16_t level, uint16_t layer);
        void SetTexture(Utilities::NameID name, RHI::Objects::Texture* texture);
        void SetImage(Utilities::NameID name, RHI::Objects::Texture* texture, uint16_t level, uint16_t layer);
        void SetImage(Utilities::NameID name, RHI::Objects::Texture* texture);

        virtual void WaitForIdle() const = 0;
        virtual void GC() = 0;

        // @TODO this ownership model is bad. 
        // Should be part of raii but destructor execution order prevents that.
        void CreateBuiltInResources();
        void ReleaseBuiltInResources();
        
        static inline Driver* Get() { return s_instance; }

        template<typename Child>
        static inline Child* GetNative()
        {
            static_assert(std::is_base_of<Driver, Child>::value, "Child doesn't derive from base!");
            return static_cast<Child*>(s_instance);
        }

        Utilities::PropertyBlock globalResources = Utilities::PropertyBlock(16384ull, 128ull);
        BuiltInResources* builtInResources;
        
        protected: inline static Driver* s_instance = nullptr;
    };
}