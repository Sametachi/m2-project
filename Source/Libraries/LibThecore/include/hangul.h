#pragma once

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#ifdef __WIN32__
#define isdigit iswdigit
#define isspace iswspace
#endif

#define ishan(ch)       (((ch) & 0xE0) > 0x90)
#define ishanasc(ch)    (isascii(ch) || ishan(ch))
#define ishanalp(ch)    (isalpha(ch) || ishan(ch))
#define isnhdigit(ch)   (!ishan(ch) && isdigit(ch))
#define isnhspace(ch)   (!ishan(ch) && isspace(ch))

	// God knows what these functions are for..
    extern const char* 	first_han(const uint8_t* str);
    extern int		check_han(const char* str);
    extern int		is_hangul(const uint8_t* str);
    extern int		under_han(const void* orig);

#define UNDER(str)	under_han(str)

#ifdef __cplusplus
};
#endif
