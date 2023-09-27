#pragma once
#include "ReferenceObject.h"
#include <string>
#include <VFE/Include/VFE.hpp>

class CResource : public CReferenceObject
{
public:
	using TType = uint32_t;

	enum EState
	{
		STATE_EMPTY,
		STATE_ERROR,
		STATE_EXIST,
		STATE_LOAD,
		STATE_FREE
	};

	// If the type matches then we are ready to use it
	static TType StringToType(const char* c_szType);
	static TType Type();

	// Managing the resources, Load/Reload/Clear/Create/Remove
	void Load();
	void Reload();
	void Clear();
	virtual bool	CreateDeviceObjects();
	virtual void	DestroyDeviceObjects();

	CResource(const FilenameWrapper& filename);
	virtual ~CResource() = default;

	// Only if we want to remove it instantly
	static void	SetDeleteImmediately(bool isSet = false);

	// Once the resource is properly loaded we are able to control it's data
	bool IsData() const;
	bool IsEmpty() const;
	bool IsType(TType type);

	const char* GetFileName() const
	{
		return m_stFileName.GetPath().c_str();
	}

	std::string GetFileNameString() const
	{
		return m_stFileName.GetPath();
	}

	const FilenameWrapper& GetFilename() const
	{
		return m_stFileName;
	}

	virtual bool OnLoad(int32_t iSize, const void* c_pvBuf) = 0;

protected:
	// Giving a name to the resource, usually always the same as the input one, but further on for Bandi it will be needed
	void SetFileName(const FilenameWrapper& filename);

	// Managing the resources, Remove, Clear, Force Remove, Init, Type?
	virtual void OnClear() = 0;
	virtual bool OnIsEmpty() const = 0;
	virtual bool OnIsType(TType type) = 0;
	virtual void OnConstruct();
	virtual void OnSelfDestruct();

	FilenameWrapper m_stFileName;
	EState me_state;

	static bool ms_bDeleteImmediately;
};
