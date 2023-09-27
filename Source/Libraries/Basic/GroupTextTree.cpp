#include "Logging.hpp"
#include "GroupTextTree.h"
#include <list>
#include <iterator>
#include <utility>

//
// GroupTextNode
//

uint32_t GroupTextNode::GetType() const
{
	return m_type;
}

std::string GroupTextNode::GetName() const
{
	return m_name;
}

GroupTextNode::GroupTextNode(uint32_t type, std::string& name, GroupTextMemory* memory): m_type(type), m_name(name), m_memory(memory)
{
}

/*virtual*/ GroupTextNode::~GroupTextNode()
{
}

//
// GroupTextList
//

GroupTextList::GroupTextList(std::string& name,GroupTextMemory* memory): GroupTextNode(kList, name, memory)
{
	// ctor
}

uint32_t GroupTextList::GetLineCount() const
{
	return m_lines.size();
}

const std::vector<std::string>& GroupTextList::GetLines() const
{
	return m_lines;
}

boost::string_view GroupTextList::GetLine(uint32_t index) const
{
	assert(index < m_lines.size() && "Out of bounds");
	return m_lines[index].c_str();
}

void GroupTextList::AddLine(std::string line)
{
	m_lines.push_back(std::move(line));
}

//
// GroupTextGroup
//

GroupTextGroup::GroupTextGroup(std::string name,GroupTextMemory* memory): GroupTextNode(kGroup, name, memory)
{
	// ctor
}

/*virtual*/ GroupTextGroup::~GroupTextGroup()
{
	auto f = [this](const ChildMap::value_type& p) 
	{
		auto node = p.second;
		if (node->GetType() == kGroup)
			m_memory->DeleteGroup(static_cast<GroupTextGroup*>(node));
		else
			m_memory->DeleteList(static_cast<GroupTextList*>(node));
	};

	std::for_each(m_children.begin(), m_children.end(), f);
}

const GroupTextGroup::ChildMap& GroupTextGroup::GetChildren() const
{
	return m_children;
}

const GroupTextGroup* GroupTextGroup::GetGroup(std::string& name) const
{
	const auto it = m_children.find(name);

	if (it != m_children.end() &&
		it->second->GetType() == kGroup)
		return static_cast<const GroupTextGroup*>(it->second);

	return nullptr;
}

const GroupTextList* GroupTextGroup::GetList(std::string& name) const
{
	const auto it = m_children.find(name);

	if (it != m_children.end() &&
		it->second->GetType() == kList)
		return static_cast<const GroupTextList*>(it->second);

	return nullptr;
}

const GroupTextList* GroupTextGroup::GetList(const char* name) const
{
	const auto it = m_children.find(name);

	if (it != m_children.end() &&
		it->second->GetType() == kList)
		return static_cast<const GroupTextList*>(it->second);

	return nullptr;
}

bool GroupTextGroup::AddChild(GroupTextNode* node)
{
	assert(node != nullptr && "NULL node given");

	auto r = m_children.emplace(node->GetName(), node);
	if (!r.second) {
		auto old = r.first->second;
		if (old->GetType() == kGroup)
			m_memory->DeleteGroup(static_cast<GroupTextGroup*>(old));
		else
			m_memory->DeleteList(static_cast<GroupTextList*>(old));

		r.first->second = node;
		return true;
	}

	return false;
}

const GroupTextGroup::PropertyMap& GroupTextGroup::GetProperties() const
{
	return m_properties;
}

std::string GroupTextGroup::GetProperty(std::string key) const
{
	const auto it = m_properties.find(key);

	if (it != m_properties.end() && !it->second.empty())
		return it->second[0];

	return "";
}

const std::vector<std::string>* GroupTextGroup::GetTokens(std::string key) const
{
	const auto it = m_properties.find(key);

	if (it != m_properties.end())
		return &it->second;

	return nullptr;
}

void GroupTextGroup::SetProperty(std::string key, std::string value)
{
	auto& vec = m_properties[key];

	vec.clear();
	vec.push_back(value);
}

void GroupTextGroup::SetProperty(std::string& key, const std::vector<std::string>& value)
{
	auto& vec = m_properties[key];

	vec.clear();
	vec.reserve(value.size());

	for (const auto& v : value)
		vec.push_back(v);
}

//
// GroupTextMemory
//

GroupTextMemory::GroupTextMemory()
{
	// ctor
}


GroupTextList* GroupTextMemory::NewList(std::string& name)
{
	return new GroupTextList(name, this);
}

GroupTextGroup* GroupTextMemory::NewGroup(std::string& name)
{
	return new GroupTextGroup(name, this);
}

void GroupTextMemory::DeleteList(GroupTextList* l)
{
	delete l;
}

void GroupTextMemory::DeleteGroup(GroupTextGroup* g)
{
	delete g;
}

//
// GroupTextReader
//

GroupTextReader::GroupTextReader(GroupTextMemory* memory): GroupTextGroup("Root", memory)
{
	// ctor
}

namespace
{
	class Parser : public GroupTextParser<Parser>
	{
	public:
		Parser(GroupTextGroup* root,GroupTextMemory* memory,std::string filename): m_memory(memory), m_parents(), m_filename(filename), m_currentGroup(root)
		{
			// ctor
		}

		// GroupTextParser<T> callbacks

		void OnError(const char* msg)
		{
			SysLog("{0}:{1} error: {2}", m_filename, m_currentLine, msg);
			m_errorOccurred = true;
		}

		void OnScopeOpen(uint32_t type, std::string name)
		{
			m_parents.push_back(m_currentGroup);

			if (type == GroupTextScope::kGroup)
			{
				auto newCurrent = m_memory->NewGroup(name);

				if (m_currentGroup->AddChild(newCurrent))
				{
#ifdef USE_GROUPTEXTTREE_DEBUG
					SysLog("{0}:{1} : warning: Group '{2}' replaced", m_filename, m_currentLine, name);
#endif
				}

				m_currentGroup = newCurrent;
			}
			else if (type == GroupTextScope::kList) 
			{
				auto newCurrent = m_memory->NewList(name);

				if (m_currentGroup->AddChild(newCurrent))
				{
#ifdef USE_GROUPTEXTTREE_DEBUG
					SysLog("{0}:{1} : warning: List '{2}' replaced", m_filename, m_currentLine, name);
#endif
				}

				m_currentList = newCurrent;
			}
		}

		void OnScopeClose()
		{
			if (m_parents.empty()) 
			{
				OnError("Too many closing braces");
				return;
			}

			m_currentGroup = m_parents.back();
			m_parents.pop_back();
		}

		void OnKeyValuePair(std::string key, std::vector<std::string> tokens)
		{
			assert(m_currentScopeType == GroupTextScope::kGroup && "Can only be used in group mode.");
			m_currentGroup->SetProperty(key, tokens);
		}

		void OnListItems(std::string& line)
		{
			assert(m_currentScopeType == GroupTextScope::kList && "Can only be used in group mode.");
			m_currentList->AddLine(line.c_str());
		}

	private:
		GroupTextMemory* m_memory;
		std::list<GroupTextGroup*> m_parents;
		std::string m_filename;

		union 
		{
			GroupTextGroup* m_currentGroup;
			GroupTextList* m_currentList;
		};
	};

}

bool GroupTextReader::LoadString(std::string data)
{
	Parser p(this, m_memory, std::string("<string>"));
	return p.Parse(data);
}

bool GroupTextReader::LoadFile(std::string filename)
{
	Parser p(this, m_memory, filename);
	return LoadGroupTextFile(filename, p);
}

