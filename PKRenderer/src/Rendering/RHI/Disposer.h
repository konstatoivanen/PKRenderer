#pragma once
#include "Utilities/Ref.h"
#include "Utilities/FenceRef.h"
#include "Utilities/VersionedObject.h"

namespace PK::Rendering::RHI
{
    class Disposer : public PK::Utilities::NoCopy
    {
        public:
            typedef void (*DeletaFunction)(void*);
            
            struct DisposeHandle
            {
                void* disposable = nullptr;
                DeletaFunction destructor = nullptr;
                Utilities::FenceRef fence{};
            };

            Disposer() {}
            ~Disposer();

            template<typename T>
            void Dispose(T* disposable, const Utilities::FenceRef& releaseFence)
            {
                if (disposable != nullptr)
                {
                    m_disposables.push_back({ disposable, [](void* v) { delete reinterpret_cast<T*>(v); }, releaseFence });
                }
            }

            template<typename T>
            void Dispose(T* disposable, DeletaFunction deleter, const Utilities::FenceRef& releaseFence)
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
