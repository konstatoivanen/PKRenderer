#pragma once
#include "Utilities/NoCopy.h"
#include "Utilities/FenceRef.h"

namespace PK::Utilities
{
    class Disposer : public PK::Utilities::NoCopy
    {
        public:
            typedef void (*Destructor)(void*);
            
            struct DisposeHandle
            {
                void* disposable = nullptr;
                Destructor destructor = nullptr;
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
            void Dispose(T* disposable, Destructor destructor, const Utilities::FenceRef& releaseFence)
            {
                if (disposable != nullptr)
                {
                    m_disposables.push_back({ disposable, destructor, releaseFence });
                }
            }

            void Prune();

        private:
            std::vector<DisposeHandle> m_disposables;
    };
}
