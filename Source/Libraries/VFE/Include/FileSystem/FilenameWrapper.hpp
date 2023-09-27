#pragma once
#include <string>
#include <cstdint>

// Simple filename wrapper class
class FilenameWrapper
{
public:
	FilenameWrapper() = default;
	FilenameWrapper(const std::string& path);
	FilenameWrapper(std::string_view path);
	FilenameWrapper(const char* path);

	FilenameWrapper& operator=(const std::string& path);
	FilenameWrapper& operator=(std::string_view path);
	FilenameWrapper& operator=(const char* path);

	void Set(const char* path, uint32_t length);

	uint64_t GetHash() const;
	const std::string& GetPath() const;

private:
	std::string m_path;
	uint64_t m_hash{};
};

class CFilename
{
public:
	CFilename() = default;
	explicit CFilename(const char* pFilename) { m_sRaw = pFilename; }
	CFilename(std::string strFilename) { m_sRaw = strFilename; }

	virtual ~CFilename() = default;

	operator const std::string() const { return m_sRaw; }
	operator std::string& () { return m_sRaw; }
	CFilename& operator=(const CFilename& r) = default;

	bool operator==(const CFilename& r) const { return m_sRaw == r.m_sRaw; }
	CFilename operator+(const CFilename& r) const { return CFilename(m_sRaw + r.m_sRaw); }
	CFilename& operator+=(const CFilename& r)
	{
		m_sRaw += r.m_sRaw;
		return *this;
	}
	const char& operator[](size_t nIdx) const { return m_sRaw[nIdx]; }
	const char* c_str() const { return m_sRaw.c_str(); }
	size_t find(const char* pcszSrc) const { return m_sRaw.find(pcszSrc); }
	bool empty() const { return m_sRaw.empty(); }
	size_t size() const { return m_sRaw.size(); }
	size_t length() const { return m_sRaw.length(); }

	std::string& GetString() { return m_sRaw; }

	void ChangeDosPath()
	{
		size_t nLength = m_sRaw.length();

		for (size_t i = 0; i < nLength; ++i)
		{
			if (m_sRaw.at(i) == '/')
				m_sRaw.at(i) = '\\';
		}
	}

	void StringPath()
	{
		size_t nLength = m_sRaw.length();

		for (size_t i = 0; i < nLength; ++i)
		{
			if (m_sRaw.at(i) == '\\')
				m_sRaw.at(i) = '/';
			else
				m_sRaw.at(i) = static_cast<char>(tolower(m_sRaw.at(i)));
		}
	}

	std::string GetName(void);
	std::string GetExtension(void);
	std::string GetPath(void);
	std::string NoExtension(void) const;
	std::string NoPath(void);

	int32_t compare(const char* s) const { return m_sRaw.compare(s); }
	friend CFilename operator+(const std::string alfa, const CFilename& beta);

	std::string m_sRaw;
};

inline CFilename operator+(const std::string alfa, const CFilename& beta)
{
	return beta + alfa;
}

inline std::string CFilename::GetName(void)
{
	std::string strName;

	size_t nLength = m_sRaw.length();

	if (nLength > 0)
	{
		size_t iExtensionStartPos = nLength - 1;

		for (size_t i = nLength - 1; i > 0; i--)
		{
			if (m_sRaw[i] == '.')
				iExtensionStartPos = i;

			if (m_sRaw[i] == '/')
			{
				strName = std::string(m_sRaw.c_str() + i + 1);
				strName.resize(iExtensionStartPos - i - 1);
				break;
			}
		}
	}

	return strName;
}

inline std::string CFilename::GetExtension(void)
{
	std::string strExtension;

	size_t nLength = m_sRaw.length();

	if (nLength > 0)
	{
		for (size_t i = nLength - 1; i > 0 && m_sRaw[i] != '/'; i--)
			if (m_sRaw[i] == '.')
			{
				strExtension = std::string(m_sRaw.c_str() + i + 1);
				break;
			}
	}

	return strExtension;
}

inline std::string CFilename::GetPath(void)
{
	char szPath[1024];
	szPath[0] = '\0';

	size_t nLength = m_sRaw.length();

	if (nLength > 0)
	{
		for (size_t i = nLength - 1; i > 0; i--)
		{
			if (m_sRaw[i] == '/' || m_sRaw[i] == '\\')
			{
				for (size_t j = 0; j < i + 1; j++)
					szPath[j] = m_sRaw[j];
				szPath[i + 1] = '\0';
				break;
			}

			if (0 == i)
				break;
		}
	}
	return szPath;
}
inline std::string CFilename::NoExtension(void) const
{
	std::size_t npos = m_sRaw.find_last_of('.');

	if (std::string::npos != npos)
		return std::string(m_sRaw, 0, npos);

	return m_sRaw;
}
inline std::string CFilename::NoPath(void)
{
	char szPath[1024];
	szPath[0] = '\0';

	size_t nLength = m_sRaw.length();

	if (nLength > 0)
	{
		strcpy_s(szPath, m_sRaw.c_str());

		for (size_t i = nLength - 1; i > 0; i--)
		{
			if (m_sRaw[i] == '/' || m_sRaw[i] == '\\')
			{
				int32_t k = 0;
				for (size_t j = i + 1; j < nLength; j++, k++)
					szPath[k] = m_sRaw[j];
				szPath[k] = '\0';
				break;
			}

			if (0 == i)
				break;
		}
	}

	return szPath;
}

class CFileNameHelper
{
public:
	static void ChangeDosPath(std::string& str)
	{
		size_t nLength = str.length();

		for (size_t i = 0; i < nLength; ++i)
		{
			if (str.at(i) == '/')
				str.at(i) = '\\';
		}
	}

	static void StringPath(std::string& str)
	{
		size_t nLength = str.length();

		for (size_t i = 0; i < nLength; ++i)
		{
			if (str.at(i) == '\\')
				str.at(i) = '/';
			else
				str.at(i) = static_cast<char>(tolower(str.at(i)));
		}
	}

	static std::string GetName(std::string& str);
	static std::string GetExtension(std::string& str);
	static std::string GetPath(std::string& str);
	static std::string NoExtension(const std::string& str);
	static std::string NoPath(std::string& str);
};

inline std::string CFileNameHelper::GetName(std::string& str)
{
	std::string strName;

	size_t nLength = str.length();

	if (nLength > 0)
	{
		size_t iExtensionStartPos = nLength - 1;

		for (size_t i = nLength - 1; i > 0; i--)
		{
			if (str[i] == '.')
				iExtensionStartPos = i;

			if (str[i] == '/')
			{
				strName = std::string(str.c_str() + i + 1);
				strName.resize(iExtensionStartPos - i - 1);
				break;
			}
		}
	}

	return strName;
}

inline std::string CFileNameHelper::GetExtension(std::string& str)
{
	std::string strExtension;

	size_t nLength = str.length();

	if (nLength > 0)
	{
		for (size_t i = nLength - 1; i > 0 && str[i] != '/'; i--)
			if (str[i] == '.')
			{
				strExtension = std::string(str.c_str() + i + 1);
				break;
			}
	}

	return strExtension;
}

inline std::string CFileNameHelper::GetPath(std::string& str)
{
	char szPath[1024];
	szPath[0] = '\0';

	size_t nLength = str.length();

	if (nLength > 0)
	{
		for (size_t i = nLength - 1; i > 0; i--)
		{
			if (str[i] == '/' || str[i] == '\\')
			{
				for (size_t j = 0; j < i + 1; j++)
					szPath[j] = str[j];
				szPath[i + 1] = '\0';
				break;
			}

			if (0 == i)
				break;
		}
	}
	return szPath;
}

inline std::string CFileNameHelper::NoExtension(const std::string& str)
{
	std::size_t npos = str.find_last_of('.');

	if (std::string::npos != npos)
		return std::string(str, 0, npos);

	return str;
}

inline std::string CFileNameHelper::NoPath(std::string& str)
{
	char szPath[1024];
	szPath[0] = '\0';

	size_t nLength = str.length();

	if (nLength > 0)
	{
		strcpy_s(szPath, str.c_str());

		for (size_t i = nLength - 1; i > 0; i--)
		{
			if (str[i] == '/' || str[i] == '\\')
			{
				int32_t k = 0;
				for (size_t j = i + 1; j < nLength; j++, k++)
					szPath[k] = str[j];
				szPath[k] = '\0';
				break;
			}

			if (0 == i)
				break;
		}
	}

	return szPath;
}