#pragma once
#include <string>
#include <windows.h>

class CFileBase
{
public:
	enum EFileMode
	{
		FILEMODE_READ = (1 << 0),
		FILEMODE_WRITE = (1 << 1)
	};

	CFileBase();
	virtual	~CFileBase();

	void Destroy();
	void Close();
	
	bool Create(const std::string& filename, EFileMode mode);
	uint32_t GetSize();
	void SeekCur(uint32_t size);
	void Seek(uint32_t offset);
	uint32_t GetPosition();

	bool Write(const void* src, uint32_t bytes);
	bool Read(void* dest, uint32_t bytes);

	const std::string& GetFileName();
	
protected:
	int32_t		m_mode;
	std::string m_filename;
	HANDLE		m_hFile;
	uint32_t	m_dwSize;
};
