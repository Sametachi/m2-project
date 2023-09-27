#pragma once
#include "GroupText.h"
#include <boost/unordered_map.hpp>
#include <vector>
#include <unordered_map>

class GroupTextGroup;
class GroupTextMemory;

/// An abstract GroupText node.
///
/// This class represents an abstract GroupText node,
/// which shouldn't be used directly. Use the two child classes
/// @c GroupTextList and @c GroupTextGroup instead.
class GroupTextNode
{
public:
	/// The two GroupText node types.
	enum Type 
	{
		kGroup,
		kList
	};

	/// Get the type of this node.
	///
	/// @return A type listed in the @c Type enum.
	uint32_t GetType() const;

	/// Get the node name:
	///
	/// @return The name of this node.
	std::string GetName() const;

protected:
	GroupTextNode(uint32_t type, std::string& name, GroupTextMemory* memory);

	virtual ~GroupTextNode();

	uint32_t m_type;
	std::string m_name;
	GroupTextMemory* m_memory;
};

/// A list element.
///
/// This class represents a list element inside a GroupText document.
///
/// @par Syntax Example
/// @code
/// List MyListName
/// {
///     Line1
///     Line2
///     Line3
///     ...
/// }
/// @endcode
class GroupTextList : public GroupTextNode
{
public:
	// undocumented
	GroupTextList(std::string& name, GroupTextMemory* memory);

	/// Get the number of lines this list has.
	///
	/// @return The number of lines.
	uint32_t GetLineCount() const;

	/// Get the list lines.
	///
	/// This function is used to retrieve a vector of all lines.
	///
	/// @return A const ref. to the line vector.
	const std::vector<std::string>& GetLines() const;

	/// Get a list line.
	///
	/// This function is used to retrieve a specific line from the list.
	///
	/// @param index The index of the line to retrieve.
	/// Must be in [0, LineCount].
	///
	/// @return A const ref. to the specified line.
	boost::string_view GetLine(uint32_t index) const;

	/// Add a line to the list.
	///
	/// This function adds the given line to this list.
	///
	/// @param line The line to add.
	void AddLine(std::string line);

private:
	std::vector<std::string> m_lines;
};

/// A group element.
///
/// This class represents a group element inside a GroupText document.
///
/// @par Syntax Example
/// @code
/// Group MyGroupName
/// {
///     Key1 Value1
///     Key1	"Value1"
///     Key1	Value1
///
///     Group SubGroupName
///     {
///         Key1 Value1
///         ...
///     }
///
///     List SubListName
///     {
///         Line1
///         ...
///     }
/// }
/// @endcode
class GroupTextGroup : public GroupTextNode
{
public:
	typedef std::unordered_map<std::string, GroupTextNode*> ChildMap;

	typedef std::unordered_map<std::string, std::vector<std::string>> PropertyMap;

	// undocumented
	GroupTextGroup(std::string name, GroupTextMemory* memory);

	/*virtual*/ ~GroupTextGroup();

	/// Get the map containing all child nodes.
	const ChildMap& GetChildren() const;

	/// Get a child group.
	const GroupTextGroup* GetGroup(std::string& name) const;

	/// Get a child list.
	const GroupTextList* GetList(std::string& name) const;
	const GroupTextList* GetList(const char* name) const;

	/// Add a child node.
	///
	/// This function adds the given child node (List or Group) to the
	/// child list, replacing any other node with the same name.
	/// Ownership is transferred to the parent @c GroupTextGroup.
	///
	/// @param node The node to insert.
	///
	/// @return A @c bool denoting whether the node replaced an existing.
	bool AddChild(GroupTextNode* node);

	/// Get the map containing all property tokens.
	const PropertyMap& GetProperties() const;

	/// Get the first token of a property.
	std::string GetProperty(std::string key) const;

	/// Get all tokens of a property.
	const std::vector<std::string>* GetTokens(std::string key) const;

	/// Set a property to a single token.
	void SetProperty(std::string key, std::string value);

	/// Set a property to a token vector.
	void SetProperty(std::string& key, const std::vector<std::string>& value);

private:
	ChildMap m_children;
	PropertyMap m_properties;
};

/// Memory manager for GroupText documents.
///
/// This class is used to new/delete GroupText nodes (lists, groups).
class GroupTextMemory
{
public:
	GroupTextMemory();


	GroupTextList* NewList(std::string& name);
	GroupTextGroup* NewGroup(std::string& name);

	void DeleteList(GroupTextList* l);
	void DeleteGroup(GroupTextGroup* g);

};

/// Read a GroupText document from a file or string.
///
/// This class is used to read a GroupText document from a file or a string.
/// It also represents the top-node in the loaded document.
class GroupTextReader : public GroupTextGroup
{
public:
	/// Construct a new GroupTextReader.
	///
	/// @param memory The @c GroupTextMemory to use for this document.
	GroupTextReader(GroupTextMemory* memory);

	/// Load a GroupText document from a string.
	///
	/// This function is used to load a GroupText document from a string.
	///
	/// @param data The complete document text.
	///
	/// @return A @c bool denoting whether the
	/// string was successfully parsed.
#undef LoadString
	bool LoadString(std::string data);

	/// Load a GroupText document from a file.
	///
	/// This function is used to load a GroupText document from a file.
	///
	/// @param filename The path of the file to load.
	///
	/// @return A @c bool denoting whether the
	/// file was successfully loaded.
	bool LoadFile(std::string filename);
};
