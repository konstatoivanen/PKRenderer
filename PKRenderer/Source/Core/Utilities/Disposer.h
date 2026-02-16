#pragma once
#include "Core/Utilities/NoCopy.h"
#include "Core/Utilities/FenceRef.h"
#include "Core/Utilities/FastList.h"

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

            Disposer(size_t initialCapacity);
            ~Disposer();

            template<typename T>
            void Dispose(T* disposable, const FenceRef& releaseFence)
            {
                Dispose(nullptr, disposable, []([[maybe_unused]] void* c, void* v) { delete reinterpret_cast<T*>(v); }, releaseFence);
            }

            void Dispose(void* context, void* disposable, Destructor destructor, const FenceRef& releaseFence);
            void Prune();

        private:
            FastList<DisposeHandle> m_disposables;
    };
}
