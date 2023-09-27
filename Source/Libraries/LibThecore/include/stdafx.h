#pragma once

#if defined(__GNUC__)
#define INLINE __inline__
#elif defined(_MSC_VER)
#define INLINE inline
#endif

#ifdef __WIN32__
#include <winsock2.h>
#include <windows.h>
#include <tchar.h>
#include <errno.h>
#include <ctime>
#include <sys/types.h>
#include <sys/stat.h>
#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <conio.h>
#include <process.h>
#include <climits>
#include <cmath>
#include <clocale>
#include <io.h>
#include <direct.h>
#include <fcntl.h>

#include "xdirent.h"
#include "xgetopt.h"

#define S_ISDIR(m)	(m & _S_IFDIR)

#define __USE_SELECT__

#define PATH_MAX _MAX_PATH

// C runtime library adjustments
#define strlcpy(dst, src, size) strncpy_s(dst, size, src, _TRUNCATE)
#define strcasecmp(s1, s2) _stricmp(s1, s2)
#define strncasecmp(s1, s2, n) _strnicmp(s1, s2, n)
#define strlcat(dst, src, size) strcat_s(dst, size, src)
#define strtok_r(s, delim, ptrptr) strtok_s(s, delim, ptrptr)
#define strdup _strdup
#define close _close
#define write _write
#define read _read
#define fileno _fileno


/*
#define strlcat(dst, src, size) strcat_s(dst, size, src)
#define strtoull(str, endptr, base) _strtoui64(str, endptr, base)
#define strtof(str, endptr) (float)strtod(str, endptr)

#define atoll(str) _atoi64(str)
#define localtime_r(timet, result) localtime_s(result, timet)
*/
#include <boost/typeof/typeof.hpp>
#define typeof(t) BOOST_TYPEOF(t)

// dummy declaration of non-supported signals
#define SIGUSR1     30  /* user defined signal 1 */
#define SIGUSR2     31  /* user defined signal 2 */

inline void usleep(unsigned long usec) {
	::Sleep(usec / 1000);
}
inline unsigned sleep(unsigned sec) {
	::Sleep(sec* 1000);
	return 0;
}
inline double rint(double x)
{
	return ::floor(x+.5);
}


#else

#ifndef __FreeBSD__
#define __USE_SELECT__
#ifdef __CYGWIN__
#define _POSIX_SOURCE 1
#endif
#endif

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <dirent.h>

#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>

#include <sys/signal.h>
#include <sys/wait.h>

#include <pthread.h>
#include <semaphore.h>

#ifdef __FreeBSD__
#include <sys/event.h>
#endif

#endif

#ifndef FALSE
#define FALSE	false
#define TRUE	(!FALSE)
#endif

#include "typedef.h"
#include "heart.h"
#include "fdwatch.h"
#include "msocket.h"
#include "kstbl.h"
#include "hangul.h"
#include "buffer.h"
#include "msignal.h"
#include "main.h"
#include "utils.h"
#include "crypt.h"
#include <Basic/Logging.hpp>
