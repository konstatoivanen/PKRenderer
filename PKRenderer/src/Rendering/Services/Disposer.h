#pragma once
#include "Utilities/Ref.h"
#include "Utilities/VersionedObject.h"
#include "Rendering/Structs/ExecutionGate.h"

namespace PK::Rendering::Services
{
    struct IDisposable : public PK::Utilities::VersionedObject
    {
        virtual ~IDisposable() = 0 {};
    };


    class Disposer : public PK::Utilities::NoCopy
    {
        struct DisposeHandle
        {
            PK::Utilities::Scope<IDisposable> disposable = nullptr;
            Structs::ExecutionGate gate{};
        };

        public:
            Disposer() {}

            template<typename T>
            void Dispose(T* disposable, const Structs::ExecutionGate& releaseGate)
            {
                static_assert(std::is_base_of<IDisposable, T>::value, "Template argument type does not derive from IService!");
                m_disposables.push_back({ PK::Utilities::Scope<IDisposable>(disposable), releaseGate });
            }

            void Prune();

        private:
            std::vector<DisposeHandle> m_disposables;
    };
}