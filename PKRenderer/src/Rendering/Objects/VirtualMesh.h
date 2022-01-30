#pragma once
#include "Rendering/Objects/Mesh.h"

namespace PK::Rendering::Objects
{
    class VirtualMesh : public Core::Services::Asset, public Core::Services::IAssetImport<Utilities::Ref<Mesh>>
    {
        friend Utilities::Ref<VirtualMesh> Core::Services::AssetImporters::Create();

        public:
            VirtualMesh();
            VirtualMesh(const SubmeshRangeAllocationInfo& data, Utilities::Ref<Mesh> mesh);
            ~VirtualMesh();

            void Import(const char* filepath, Utilities::Ref<Mesh>* pParams) override final;
            inline Mesh* GetBaseMesh() const { return m_mesh.get(); }
            uint32_t GetSubmeshIndex(uint32_t submesh) const;
            uint32_t GetBaseSubmeshIndex() const { return m_submeshIndices.at(0); }
            inline const uint32_t GetSubmeshCount() const { return glm::max(1, (int)m_submeshIndices.size()); }

        private:
            Utilities::Ref<Mesh> m_mesh = nullptr;
            SubMesh m_fullRange{};
            std::vector<uint32_t> m_submeshIndices;
    };
}