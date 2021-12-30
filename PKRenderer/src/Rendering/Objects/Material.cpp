#include "PrecompiledHeader.h"
#include "Material.h"
#include "Core/YamlSerializers.h"
#include <yaml-cpp/yaml.h>

using namespace PK::Core;
using namespace PK::Utilities;
using namespace PK::Rendering::Objects;

namespace YAML
{
	using namespace PK::Math;

	Emitter& operator<<(Emitter& out, const float3& v)
	{
		out << Flow;
		out << BeginSeq << v.x << v.y << v.z << EndSeq;
		return out;
	}

	Emitter& operator<<(Emitter& out, const float4& v)
	{
		out << Flow;
		out << BeginSeq << v.x << v.y << v.z << v.w << EndSeq;
		return out;
	}
}

namespace PK::Rendering::Objects
{
	void Material::CopyTo(char* dst, BindSet<Texture>* textureSet) const
	{
		auto& layout = m_shader->GetMaterialPropertyLayout();

		memcpy(dst, m_buffer, layout.GetAlignedStride());

		for (auto& element : layout)
		{
			switch (element.Type)
			{
				case ElementType::Texture2DHandle: 
					auto texIndex = textureSet->Set(*Get<Texture*>(element.NameHashId));
					memcpy(dst + element.AlignedOffset, &texIndex, sizeof(int32_t));
					break;
			}
		}
	}

	void Material::Import(const char* filepath)
    {
        Clear();

        YAML::Node root = YAML::LoadFile(filepath);

		auto data = root["Material"];
		PK_THROW_ASSERT(data, "Could not locate material (%s) header in file.", filepath);
		
		auto shaderPathProp = data["Shader"];
		PK_THROW_ASSERT(shaderPathProp, "Material (%s) doesn't define a shader.", filepath);

		auto shaderPath = shaderPathProp.as<std::string>();
		m_shader = Application::GetService<AssetDatabase>()->Load<Shader>(shaderPath);
		m_cachedShaderAssetId = m_shader->GetAssetID();

		auto& propertyLayout = m_shader->GetMaterialPropertyLayout();

		PK_THROW_ASSERT(propertyLayout.size() > 0, "Shader is doesn't support materials!");

		ReserveLayout(propertyLayout);

		auto keywords = data["Keywords"];
		
		if (keywords)
		{
			for (auto keyword : keywords)
			{
				Set<bool>(StringHashID::StringToID(keyword.as<std::string>()), true);
			}
		}

		auto properties = data["Properties"];

		if (properties)
		{
			for (auto property : properties)
			{
				auto propertyName = property.first.as<std::string>();

				auto type = property.second["Type"];

				if (!type)
				{
					continue;
				}

				auto nameHash = StringHashID::StringToID(propertyName);
				auto typeName = type.as<std::string>();
				auto elementType = PK::Assets::GetElementType(typeName.c_str());
				auto values = property.second["Value"];

				switch (elementType)
				{
					case ElementType::Float: Set(nameHash, values.as<float>()); break;
					case ElementType::Float2: Set(nameHash, values.as<float2>()); break;
					case ElementType::Float3: Set(nameHash, values.as<float3>()); break;
					case ElementType::Float4: Set(nameHash, values.as<float4>()); break;
					case ElementType::Float2x2: Set(nameHash, values.as<float2x2>()); break;
					case ElementType::Float3x3: Set(nameHash, values.as<float3x3>()); break;
					case ElementType::Float4x4: Set(nameHash, values.as<float4x4>()); break;
					case ElementType::Int: Set(nameHash, values.as<int>()); break;
					case ElementType::Int2: Set(nameHash, values.as<int2>()); break;
					case ElementType::Int3: Set(nameHash, values.as<int3>()); break;
					case ElementType::Int4: Set(nameHash, values.as<int4>()); break;
					case ElementType::Texture2DHandle: Set(nameHash, values.as<Texture*>()); break;
					case ElementType::Texture3DHandle: Set(nameHash, values.as<Texture*>()); break;
					case ElementType::TextureCubeHandle: Set(nameHash, values.as<Texture*>()); break;
				}
			}
		}
	}
}

template<>
bool AssetImporters::IsValidExtension<Material>(const std::filesystem::path& extension) { return extension.compare(".material") == 0; }

template<>
Ref<Material> AssetImporters::Create() { return CreateRef<Material>(); }