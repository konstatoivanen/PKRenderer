#pragma once
#include "Rendering/Objects/Mesh.h"

namespace PK::Rendering::Objects
{
    using namespace PK::Core::Services;

    class VirtualMesh : public Asset
    {
        friend Ref<VirtualMesh> AssetImporters::Create();

        public:
            VirtualMesh();
            VirtualMesh(const SubmeshRangeAllocationInfo& data, Ref<Mesh> mesh);
            ~VirtualMesh();

            void Import(const char* filepath, void* pParams) override final;
            inline Mesh* GetBaseMesh() const { return m_mesh.get(); }
            uint32_t GetSubmeshIndex(uint32_t submesh) const;
            uint32_t GetBaseSubmeshIndex() const { return m_submeshIndices.at(0); }
            inline const uint GetSubmeshCount() const { return glm::max(1, (int)m_submeshIndices.size()); }

        private:
            Ref<Mesh> m_mesh = nullptr;
            SubMesh m_fullRange{};
            std::vector<uint32_t> m_submeshIndices;
    };
}