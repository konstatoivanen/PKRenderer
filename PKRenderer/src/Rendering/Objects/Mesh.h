#pragma once
#include "Core/Services/AssetDatabase.h"
#include "Rendering/Objects/Buffer.h"
#include "Rendering/Structs/StructsCommon.h"

namespace PK::Rendering::Objects
{
    using namespace PK::Core::Services;

    struct SubMesh
    {
        uint32_t firstVertex;
        uint32_t vertexCount;
        uint32_t firstIndex;
        uint32_t indexCount;

        constexpr bool operator < (const SubMesh& other)
        {
            if (firstVertex != other.firstVertex) return firstVertex < other.firstVertex;
            if (firstIndex != other.firstIndex) return firstIndex < other.firstIndex;
            if (vertexCount != other.vertexCount) return vertexCount < other.vertexCount;
            if (indexCount != other.indexCount) return indexCount < other.indexCount;
            return false;
        }
    };

    struct SubmeshRangeAllocationInfo
    {
        void* pVertices;
        void* pIndices;
        BufferLayout vertexLayout;
        SubMesh* pSubmeshes;
        BoundingBox* pBoundingBoxes;
        ElementType indexType;
        uint32_t vertexCount;
        uint32_t indexCount;
        uint32_t submeshCount;
    };

    class Mesh : public Asset
    {
        friend Ref<Mesh> AssetImporters::Create();

        public:
            Mesh();
            Mesh(const Ref<Buffer>& vertexBuffer, const Ref<Buffer>& indexBuffer);
            Mesh(const Ref<Buffer>& vertexBuffer, const Ref<Buffer>& indexBuffer, const BoundingBox& bounds);

            void Import(const char* filepath, void* pParams) override final;

            /// Allocates a submesh range by appending submitted data to the first assigned vertex buffer.
            /// - Requires the first vertex buffer to be virtual.
            /// - Validates data interleaving using the submitted layout, throws an exception upon missmatch.
            void AllocateSubmeshRange(const SubmeshRangeAllocationInfo& allocationInfo,
                                      SubMesh* outAllocationRange,
                                      uint32_t* outSubmeshIndices);

            void DeallocateSubmeshRange(const SubMesh& allocationRange);

            void AddVertexBuffer(const Ref<Buffer>& vertexBuffer);
            void SetIndexBuffer(const Ref<Buffer>& indexBuffer) { m_indexBuffer = indexBuffer; }

            void SetSubMeshes(const SubMesh* submeshes,
                              const BoundingBox* boundingBoxes, 
                              size_t submeshCount, 
                              size_t boundCount);

            inline void SetSubMeshes(const std::initializer_list<SubMesh>& submeshes,
                                     const std::initializer_list<BoundingBox>& boundingBoxes)
            {
                SetSubMeshes(submeshes.begin(), 
                             boundingBoxes.begin(), 
                             submeshes.end() - submeshes.begin(), 
                             boundingBoxes.end() - boundingBoxes.begin());
            }

            inline void SetSubMeshes(const std::vector<SubMesh>& submeshes,
                                     const std::vector<BoundingBox>& boundingBoxes)
            {
                SetSubMeshes(submeshes.data(),
                    boundingBoxes.data(),
                    submeshes.size(),
                    boundingBoxes.size());
            }

            constexpr const std::vector<Ref<Buffer>>& GetVertexBuffers() const { return m_vertexBuffers; }
            const BufferLayout& GetDefaultLayout() const { return m_vertexBuffers.at(0)->GetLayout(); }
            const Buffer* GetVertexBuffer() const { return m_vertexBuffers.at(0).get(); }
            const Buffer* GetVertexBuffer(uint index) const { return m_vertexBuffers.at(index).get(); }
            const Buffer* GetIndexBuffer() const { return m_indexBuffer.get(); }
            const SubMesh GetSubmesh(int submesh) const;
            inline const uint GetSubmeshCount() const { return glm::max(1, (int)m_submeshes.size()); }
            const BoundingBox& GetBounds(int submesh) const;
            constexpr const BoundingBox& GetBounds() const { return m_fullBounds; };

        private:
            std::vector<Ref<Buffer>> m_vertexBuffers;
            Ref<Buffer> m_indexBuffer;
            std::vector<SubMesh> m_submeshes;
            std::vector<BoundingBox> m_boundingBoxes;
            BoundingBox m_fullBounds = BoundingBox::GetMinBounds();
    };
}