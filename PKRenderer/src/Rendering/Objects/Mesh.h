#pragma once
#include "Core/AssetDatabase.h"
#include "Rendering/Objects/Buffer.h"
#include "Rendering/Structs/StructsCommon.h"

namespace PK::Rendering::Objects
{
    using namespace PK::Core;

    class Mesh : public Asset
    {
        friend Ref<Mesh> AssetImporters::Create();

        public:
            Mesh();
            Mesh(const Ref<Buffer>& vertexBuffer, const Ref<Buffer>& indexBuffer);
            Mesh(const Ref<Buffer>& vertexBuffer, const Ref<Buffer>& indexBuffer, const BoundingBox& localBounds);

            void Import(const char* filepath) override final;

            void AddVertexBuffer(const Ref<Buffer>& vertexBuffer);
            void SetIndexBuffer(const Ref<Buffer>& indexBuffer) { m_indexBuffer = indexBuffer; }

            inline void SetSubMeshes(const std::initializer_list<IndexRange>& indexRanges) { m_indexRanges = indexRanges; }
            inline void SetSubMeshes(const std::vector<IndexRange>& indexRanges) { m_indexRanges = indexRanges; }

            constexpr const std::vector<Ref<Buffer>>& GetVertexBuffers() const { return m_vertexBuffers; }
            const Buffer* GetVertexBuffer(uint index) const { return m_vertexBuffers.at(index).get(); }
            const Buffer* GetIndexBuffer() const { return m_indexBuffer.get(); }
            const IndexRange GetSubmesh(int submesh) const;
            inline const uint GetSubmeshCount() const { return glm::max(1, (int)m_indexRanges.size()); }
            constexpr const BoundingBox& GetLocalBounds() const { return m_localBounds; }
            inline void SetLocalBounds(const BoundingBox& bounds) { m_localBounds = bounds; }

        private:
            std::vector<Ref<Buffer>> m_vertexBuffers;
            Ref<Buffer> m_indexBuffer;
            std::vector<IndexRange> m_indexRanges;
            BoundingBox m_localBounds;
    };
}