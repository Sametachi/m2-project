#pragma once
#include <string>
#include <algorithm>

/*----- atoi function -----*/
inline bool str_is_number(const char* in)
{
	if (0 == in || 0 == in[0])
		return false;

	auto len = strlen(in);
	for (uint32_t i = 0; i < len; ++i)
	{
		if ((in[i] < '0' || in[i] > '9') && (i > 0 || in[i] != '-'))
			return false;
	}

	return true;
}

inline bool str_to_number(bool& out, const char* in)
{
	if (0 == in || 0 == in[0])
		return false;

	out = (strtol(in, nullptr, 10) != 0);
	return true;
}

inline bool str_to_bool(bool& out, const std::string& in)
{
	out = in.at(0) == '1';
	return true;
}

inline bool str_to_number(char& out, const char* in)
{
	if (0 == in || 0 == in[0])
		return false;

	out = static_cast<char>(strtol(in, nullptr, 10));
	return true;
}

inline bool str_to_number(uint8_t& out, const char* in)
{
	if (0 == in || 0 == in[0])
		return false;

	out = static_cast<uint8_t>(strtoul(in, nullptr, 10));
	return true;
}

inline bool str_to_number(int16_t& out, const char* in)
{
	if (0 == in || 0 == in[0])
		return false;

	out = static_cast<int16_t>(strtol(in, nullptr, 10));
	return true;
}

inline bool str_to_number(uint16_t& out, const char* in)
{
	if (0 == in || 0 == in[0])
		return false;

	out = static_cast<uint16_t>(strtoul(in, nullptr, 10));
	return true;
}

inline bool str_to_number(int32_t& out, const char* in)
{
	if (0 == in || 0 == in[0])
		return false;

	out = static_cast<int32_t>(strtol(in, nullptr, 10));
	return true;
}

inline bool str_to_number(uint32_t& out, const char* in)
{
	if (0 == in || 0 == in[0])
		return false;

	out = static_cast<uint32_t>(strtoul(in, nullptr, 10));
	return true;
}

inline bool str_to_number(int64_t& out, const char* in)
{
	if (0 == in || 0 == in[0])
		return false;

	out = static_cast<int64_t>(strtoull(in, nullptr, 10));
	return true;
}

inline bool str_to_number(uint64_t& out, const char* in)
{
	if (0 == in || 0 == in[0])
		return false;

	out = static_cast<uint64_t>(strtoull(in, nullptr, 10));
	return true;
}

inline bool str_to_number(float& out, const char* in)
{
	if (0 == in || 0 == in[0])
		return false;

	out = static_cast<float>(std::stof(in));
	return true;
}

inline bool str_to_number(double& out, const char* in)
{
	if (0 == in || 0 == in[0])
		return false;

	out = static_cast<double>(strtod(in, nullptr));
	return true;
}

inline bool str_to_float(float& out, const char* in)
{
	char* end;
	double ret = (double)strtod(in, &end);
	bool parsed = (*end == '\0');

	out = parsed ? (float)ret : 0.0f;
	return parsed;
}

inline bool str_to_double(double& out, const char* in)
{
	char* end;
	double ret = (double)strtod(in, &end);
	bool parsed = (*end == '\0');

	out = parsed ? ret : 0;
	return parsed;
}

inline bool str_to_number(long double& out, const char* in)
{
	if (0 == in || 0 == in[0])
		return false;

	out = static_cast<long double>(strtold(in, nullptr));
	return true;
}

#ifdef __FreeBSD__
inline bool str_to_number(int32_t double& out, const char* in)
{
	if (0 == in || 0 == in[0])	return false;

	out = (int32_t double) strtold(in, nullptr);
	return true;
}
#endif

inline bool is_positive_number(const std::string& str)
{
	if (str.empty() || ((!isdigit(str[0])) && (str[0] != '-') && (str[0] != '+')))
		return false;

	char* p;
	strtol(str.c_str(), &p, 10);

	//Finally, check that the referenced pointer points to the end of the string. If that happens, said string is a number.
	return (*p == 0);
}
/*----- atoi function -----*/
