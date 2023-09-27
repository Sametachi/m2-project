#pragma once
#include "GroupTextTree.h"
//#include <Eigen/Core>

#ifdef _WIN641
#include "../../Extern/Graphic/DirectX/Include/d3dx9math.h"
#endif

bool GetGroupProperty(const GroupTextGroup* group, const std::string& name, std::string& value);

template <typename T>
typename boost::enable_if<boost::is_integral<T>, bool>::type GetGroupProperty(const GroupTextGroup* group, std::string name, T& val)
{
	const auto& prop = group->GetProperty(name);
	if (prop.empty())
		return false;

	val = std::stoi(prop);

	return true;
}

template <typename T>
typename boost::enable_if<boost::is_floating_point<T>, bool>::type GetGroupProperty(const GroupTextGroup* group, std::string name, T& val)
{
	const auto& prop = group->GetProperty(name);
	if (prop.empty())
		return false;

	val = std::stof(prop);

	return true;
}

#ifdef _WIN641
bool GetGroupProperty(const GroupTextGroup* group, const std::string& name, D3DXVECTOR3& vec);
#endif
//bool GetGroupProperty(const GroupTextGroup* group, std::string name, Eigen::Vector3f& vec);
//bool GetGroupProperty(const GroupTextGroup* group, std::string name, Eigen::Vector2f& vec);

template <size_t N>
bool VerifyRequiredPropertiesPresent(const GroupTextGroup* group, const std::string_view(&properties)[N])
{
	for (size_t i = 0; i != N; ++i)
	{
		const auto& prop = group->GetProperty(std::string(properties[i].begin(), properties[i].end()));
		if (!prop.empty())
			continue;

#ifdef USE_GROUPTEXTTREE_DEBUG
		ConsoleLog("Required property {0} is missing", properties[i]);
#endif
		return false;
	}

	return true;
}