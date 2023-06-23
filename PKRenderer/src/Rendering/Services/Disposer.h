#pragma once
#include "Utilities/Ref.h"
#include "Utilities/VersionedObject.h"
#include "Rendering/Structs/FenceRef.h"

namespace PK::Rendering::Services
{
    class Disposer : public PK::Utilities::NoCopy
    {
        public:
            typedef void (*DeletaFunction)(void*);
            
            struct DisposeHandle
            {
                void* disposable = nullptr;
                DeletaFunction destructor = nullptr;
                Structs::FenceRef fence{};
            };

            Disposer() {}
            ~Disposer();

            template<typename T>
            void Dispose(T* disposable, const Structs::FenceRef& releaseFence)
            {
                if (disposable != nullptr)
                {
                    m_disposables.push_back({ disposable, [](void* v) { delete reinterpret_cast<T*>(v); }, releaseFence });
                }
            }

            template<typename T>
            void Dispose(T* disposable, DeletaFunction deleter, const Structs::FenceRef& releaseFence)
            {
                if (disposable != nullptr)
                {
                    m_disposables.push_back({ disposable, deleter, releaseFence });
                }
            }

            void Prune();

        private:
            std::vector<DisposeHandle> m_disposables;
    };
}