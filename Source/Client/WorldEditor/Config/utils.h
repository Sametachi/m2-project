/*----- atoi function -----*/
extern "C"{
#include "strtof.h"
}
#define strtoll     _strtoi64
#define strtoull    _strtoui64
inline bool str_to_number (bool& out, const char *in)
{
	if (0==in || 0==in[0])	return false;

	out = (strtol(in, NULL, 10) != 0);
	return true;
}

inline bool str_to_number (char& out, const char *in)
{
	if (0==in || 0==in[0])	return false;

	out = (char) strtol(in, NULL, 10);
	return true;
}

inline bool str_to_number (uint8_t& out, const char *in)
{
	if (0==in || 0==in[0])	return false;

	out = (uint8_t) strtoul(in, NULL, 10);
	return true;
}

inline bool str_to_number (int16_t& out, const char *in)
{
	if (0==in || 0==in[0])	return false;

	out = (int16_t) strtol(in, NULL, 10);
	return true;
}

inline bool str_to_number (uint16_t& out, const char *in)
{
	if (0==in || 0==in[0])	return false;

	out = (uint16_t) strtoul(in, NULL, 10);
	return true;
}

inline bool str_to_number (int32_t& out, const char *in)
{
	if (0==in || 0==in[0])	return false;

	out = (int32_t) strtol(in, NULL, 10);
	return true;
}

inline bool str_to_number (uint32_t& out, const char *in)
{
	if (0==in || 0==in[0])	return false;

	out = (uint32_t) strtoul(in, NULL, 10);
	return true;
}

inline bool str_to_number (int32_t& out, const char *in)
{
	if (0==in || 0==in[0])	return false;

	out = (int32_t) strtol(in, NULL, 10);
	return true;
}

inline bool str_to_number (uint32_t& out, const char *in)
{
	if (0==in || 0==in[0])	return false;

	out = (uint32_t) strtoul(in, NULL, 10);
	return true;
}

inline bool str_to_number (int64_t& out, const char *in)
{
	if (0==in || 0==in[0])	return false;

	out = (int64_t) strtoull(in, NULL, 10);
	return true;
}

inline bool str_to_number (float& out, const char *in)
{
	if (0==in || 0==in[0])	return false;

	out = (float) strtof(in, NULL);
	return true;
}

inline bool str_to_number (double& out, const char *in)
{
	if (0==in || 0==in[0])	return false;

	out = (double) strtod(in, NULL);
	return true;
}

#ifdef __FreeBSD__
inline bool str_to_number (int32_t double& out, const char *in)
{
	if (0==in || 0==in[0])	return false;

	out = (int32_t double) strtold(in, NULL);
	return true;
}
#endif


/*----- atoi function -----*/
