#pragma once
#include "Core/Application.h"
#include "Core/ConsoleCommandBinding.h"
#include "Rendering/Objects/Texture.h"
#include "Math/Types.h"
#include <yaml-cpp/yaml.h>

namespace YAML
{
	using namespace PK::Utilities;
	using namespace PK::Core::Services;
	using namespace PK::Rendering::Objects;
	using namespace PK::Math;

	#define DECLARE_VECTOR_CONVERTER(type, count)				\
	template<>													\
	struct convert<type##count>									\
	{															\
		static Node encode(const type##count & rhs)				\
		{														\
			Node node;											\
			for (auto i = 0; i < count; ++i)					\
			{													\
				node.push_back(rhs[i]);							\
			}													\
			node.SetStyle(EmitterStyle::Flow);					\
			return node;										\
		}														\
																\
		static bool decode(const Node& node, type##count & rhs)	\
		{														\
			if (!node.IsSequence() || node.size() != count)		\
			{													\
				return false;									\
			}													\
																\
			for (auto i = 0; i < count; ++i)					\
			{													\
				rhs[i] = node[i].as<type>();					\
			}													\
																\
			return true;										\
		}														\
	};															\

	#define DECLARE_MATRIX_CONVERTER(type, countx, county)					\
	template<>																\
	struct convert<type##countx##x##county>									\
	{																		\
		static Node encode(const type##countx##x##county & rhs)				\
		{																	\
			Node node;														\
			for (auto i = 0; i < countx; ++i)								\
			for (auto j = 0; j < county; ++j)								\
			{																\
				node.push_back(rhs[i][j]);									\
			}																\
			node.SetStyle(EmitterStyle::Flow);								\
			return node;													\
		}																	\
																			\
		static bool decode(const Node& node, type##countx##x##county & rhs)	\
		{																	\
			if (!node.IsSequence() || node.size() != countx * county)		\
			{																\
				return false;												\
			}																\
																			\
			for (auto i = 0; i < countx; ++i)								\
			for (auto j = 0; j < county; ++j)								\
			{																\
				rhs[i][j] = node[i * county + j].as<type>();				\
			}																\
																			\
			return true;													\
		}																	\
	};																		\

	DECLARE_VECTOR_CONVERTER(float, 2)
	DECLARE_VECTOR_CONVERTER(float, 3)
	DECLARE_VECTOR_CONVERTER(float, 4)
	DECLARE_VECTOR_CONVERTER(int, 2)
	DECLARE_VECTOR_CONVERTER(int, 3)
	DECLARE_VECTOR_CONVERTER(int, 4)
	DECLARE_VECTOR_CONVERTER(uint, 2)
	DECLARE_VECTOR_CONVERTER(uint, 3)
	DECLARE_VECTOR_CONVERTER(uint, 4)

	DECLARE_MATRIX_CONVERTER(float, 2, 2)
	DECLARE_MATRIX_CONVERTER(float, 3, 3)
	DECLARE_MATRIX_CONVERTER(float, 4, 4)
	DECLARE_MATRIX_CONVERTER(float, 3, 4)

	#undef DECLARE_VECTOR_CONVERTER
	#undef DECLARE_MATRIX_CONVERTER

	template<>
	struct convert<Texture*>
	{
		static Node encode(const Texture*& rhs)
		{
			Node node;
			node.push_back(rhs->GetFileName());
			node.SetStyle(EmitterStyle::Default);
			return node;
		}

		static bool decode(const Node& node, Texture*& rhs)
		{
			auto path = node.as<std::string>();
			rhs = PK::Core::Application::GetService<AssetDatabase>()->Load<PK::Rendering::Objects::Texture>(path);
			return true;
		}
	};

	template<>
	struct convert<PK::Core::ConsoleCommandBinding>
	{
		static Node encode(const PK::Core::ConsoleCommandBinding& rhs)
		{
			Node node;

			node.push_back(Input::KeyToString(rhs.keycode));
			node.push_back(rhs.command);
			node.SetStyle(EmitterStyle::Flow);
			return node;
		}

		static bool decode(const Node& node, PK::Core::ConsoleCommandBinding& rhs)
		{
			if (!node.IsSequence() || node.size() != 2)
			{
				return false;
			}

			rhs.keycode = Input::StringToKey(node[0].as<std::string>());
			rhs.command = node[1].as<std::string>();
			return true;
		}
	};

	template<>
	struct convert<PK::Core::ConsoleCommandBindList>
	{
		static Node encode(const PK::Core::ConsoleCommandBindList& rhs)
		{
			Node node;
			for (auto& arg : rhs)
			{
				node.push_back(arg);
			}

			node.SetStyle(EmitterStyle::Default);
			return node;
		}

		static bool decode(const Node& node, PK::Core::ConsoleCommandBindList& rhs)
		{
			for (auto i = 0; i < node.size(); ++i)
			{
				rhs.push_back(node[i].as<PK::Core::ConsoleCommandBinding>());
			}

			return true;
		}
	};

	struct ParsableValue
	{
		virtual void Parse(const YAML::Node& parent) = 0;
		virtual void TryParse(const YAML::Node& parent) = 0;
	};

	template<typename T>
	struct BoxedValue : ParsableValue
	{
		std::string key;
		T value;
		T defaultValue;

		BoxedValue(const char* ckey, const T& initialValue) : key(ckey), value(initialValue), defaultValue(initialValue) {}

		inline void Parse(const YAML::Node& parent) final { value = parent[key.c_str()] ? parent[key.c_str()].as<T>() : defaultValue; }

		inline void TryParse(const YAML::Node& parent) final { if (parent[key.c_str()]) value = parent[key.c_str()].as<T>(); }

		operator T& () { return value; }
		operator T() const { return value; }
	};

	struct YamlValueList
	{
		std::vector<ParsableValue*> values;

		inline void Load(const std::string& filepath)
		{
			auto properties = YAML::LoadFile(filepath);
			PK_THROW_ASSERT(properties, "Failed to open config file at: %s", filepath.c_str());
			Parse(properties);
		}

		inline void Parse(const YAML::Node& parent)
		{
			for (auto& value : values)
			{
				value->Parse(parent);
			}
		}

		inline void TryParse(const YAML::Node& parent)
		{
			for (auto& value : values)
			{
				value->TryParse(parent);
			}
		}
	};
}