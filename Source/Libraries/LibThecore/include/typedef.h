#pragma once

struct timezone 
{
    int32_t tz_minuteswest; /* minutes west of Greenwich */
    int32_t tz_dsttime;     /* type of dst correction */
};

typedef SOCKET socket_t;

#if !defined(_W64)
#if !defined(__midl) && (defined(_X86_) || defined(_M_IX86)) && _MSC_VER >= 1300
#define _W64 __w64
#else
#define _W64
#endif
#endif

#ifdef _WIN64
typedef __int64 ssize_t;
#else
typedef _W64 int ssize_t;
#endif

// Fixed-size integer types
#if defined(_MSC_VER) && (_MSC_VER >= 1300)
typedef __int32 int32_t;
typedef unsigned __int32 uint32_t;
typedef __int64 int64_t;
typedef unsigned __int64 uint64_t;
#endif

typedef unsigned int uint;
