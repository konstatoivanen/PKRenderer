#pragma once
#include "Core/Utilities/NoCopy.h"
#include "Core/Utilities/FenceRef.h"

namespace PK
{
    class Disposer : public NoCopy
    {
        public:
            typedef void (*Destructor)(void*, void*);
            
            struct DisposeHandle
            {
                void* context = nullptr;
                void* disposable = nullptr;
                Destructor destructor = nullptr;
                FenceRef fence{};
            };

            Disposer() { m_disposables.reserve(256u); }
            ~Disposer();

            template<typename T>
            void Dispose(T* disposable, const FenceRef& releaseFence)
            {
                if (disposable != nullptr)
                {
                    m_disposables.push_back({ nullptr, disposable, []([[maybe_unused]] void* c, void* v) { delete reinterpret_cast<T*>(v); }, releaseFence });
                }
            }

            void Dispose(void* context, void* disposable, Destructor destructor, const FenceRef& releaseFence);
            void Prune();

        private:
            std::vector<DisposeHandle> m_disposables;
    };
}
