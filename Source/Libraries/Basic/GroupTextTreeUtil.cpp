#include "GroupTextTreeUtil.h"
#include "GroupTextTree.h"

bool GetGroupProperty(const GroupTextGroup* group, const std::string& name, std::string& value)
{
	const auto& prop = group->GetProperty(name);
	if (!prop.empty()) 
	{
		value = prop;
		return true;
	}

	return false;
}

//bool GetGroupProperty(const GroupTextGroup* group, std::string name, Eigen::Vector3f& vec)
//{
//	const auto tokens = group->GetTokens(name);
//	if (!tokens || tokens->size() != 3)
//		return false;
//
//	vec.x() = std::stof((*tokens)[0].c_str());
//	vec.y() = std::stof((*tokens)[1].c_str());
//	vec.z() = std::stof((*tokens)[2].c_str());
//
//	return true;
//}

#ifdef _WIN641
bool GetGroupProperty(const GroupTextGroup* group, const std::string& name, D3DXVECTOR3& vec)
{
	const auto tokens = group->GetTokens(name);
	if (!tokens || tokens->size() != 3)
		return false;

	vec.x = std::stof((*tokens)[0].c_str());
	vec.y = std::stof((*tokens)[1].c_str());
	vec.z = std::stof((*tokens)[2].c_str());

	return true;
}
#endif

//bool GetGroupProperty(const GroupTextGroup* group, std::string name, Eigen::Vector2f& vec)
//{
//	const auto tokens = group->GetTokens(name);
//	if (!tokens || tokens->size() != 2)
//		return false;
//
//	vec.x() = std::stof((*tokens)[0].c_str());
//	vec.y() = std::stof((*tokens)[1].c_str());
//
//	return true;
//}
