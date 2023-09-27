#define __LIBTHECORE__
#include "stdafx.h"

/* Forwards */
void socket_lingeron(socket_t s);
void socket_lingeroff(socket_t s);
void socket_timeout(socket_t s, long sec, long usec);
void socket_reuse(socket_t s);
void socket_keepalive(socket_t s);

int socket_udp_read(socket_t desc, char* read_point, size_t space_left, struct sockaddr* from, socklen_t* fromlen)
{
    /*
       ssize_t recvfrom(int s, void* buf, size_t len, int flags, struct sockaddr* from, socklen_t* fromlen);
     */
    ssize_t ret;
    ret = recvfrom(desc, read_point, space_left, 0, from, fromlen);
		return (ret);
}

int socket_read(socket_t desc, char* read_point, size_t space_left)
{
    int	ret;

    ret = recv(desc, read_point, space_left, 0);

    if (ret > 0)
		return ret;

    if (ret == 0)	// Á¤»óÀûÀ¸·Î Á¢¼Ó ²÷±è
	return -1;

#ifdef EINTR            /* Interrupted system call - various platforms */
    if (errno == EINTR)
		return (0);
#endif

#ifdef EAGAIN           /* POSIX */
    if (errno == EAGAIN)
		return (0);
#endif

#ifdef EWOULDBLOCK      /* BSD */
    if (errno == EWOULDBLOCK)
		return (0);
#endif /* EWOULDBLOCK */

#ifdef EDEADLK          /* Macintosh */
    if (errno == EDEADLK)
		return (0);
#endif

#ifdef __WIN32__
	int wsa_error = WSAGetLastError();
	if (wsa_error == WSAEWOULDBLOCK) {
		return 0;
	}
	TraceLog("socket_read: WSAGetLastError returned {}", wsa_error);
#endif

    TraceLog("about to lose connection");
    return -1;
}


int socket_write_tcp(socket_t desc, const char* txt, int length)
{
    int bytes_written = send(desc, txt, length, 0);

    if (bytes_written > 0)
		return (bytes_written);

    if (bytes_written == 0)
		return -1;

#ifdef EAGAIN           /* POSIX */
    if (errno == EAGAIN)
		return 0;
#endif

#ifdef EWOULDBLOCK      /* BSD */
    if (errno == EWOULDBLOCK)
		return 0;
#endif

#ifdef EDEADLK          /* Macintosh */
    if (errno == EDEADLK)
		return 0;
#endif

#ifdef __WIN32__
	int wsa_error = WSAGetLastError();
	if (wsa_error == WSAEWOULDBLOCK) {
		return 0;
	}
	TraceLog("socket_write_tcp: WSAGetLastError returned {}", wsa_error);
#endif

    /* Looks like the error was fatal.  Too bad. */
    return -1;
}


int socket_write(socket_t desc, const char* data, size_t length)
{
    size_t	total;
    int		bytes_written;

    total = length;

    do
    {
        if ((bytes_written = socket_write_tcp(desc, data, total)) < 0)
        {
#ifdef EWOULDBLOCK
            if (errno == EWOULDBLOCK)
                errno = EAGAIN;
#endif
            if (errno == EAGAIN)
            {
                TraceLog("socket write would block, about to close!");
            }
            else
            {
                TraceLog("write to desc error");   // 'Normal' The connection from the other side is cut off.
            }

			return -1;
		}
		else
		{
			data        += bytes_written;
			total       -= bytes_written;
		}
    }
    while (total > 0);

    return 0;
}

int socket_bind(const char* ip, int port, int protocol)
{
    int                 s;
#ifdef __WIN32
    SOCKADDR_IN			sa;
#else
    struct sockaddr_in  sa;
#endif

    if ((s = socket(AF_INET, protocol, 0)) < 0) 
    {
		SysLog("socket: {}", strerror(errno));
		return 0;
    }

    socket_reuse(s);
#ifndef __WIN32__
    socket_lingeroff(s);
#else
	// Winsock2: SO_DONTLINGER, SO_KEEPALIVE, SO_LINGER, and SO_OOBINLINE are 
	// not supported on sockets of type SOCK_DGRAM
	if (protocol == SOCK_STREAM) {
		socket_lingeroff(s);
	}
#endif

    memset(&sa, 0, sizeof(sa));
    sa.sin_family	= AF_INET;
    sa.sin_addr.s_addr	= inet_addr(ip);
    sa.sin_port		= htons((unsigned short) port);

    if (bind(s, (struct sockaddr*) &sa, sizeof(sa)) < 0)
    {
		SysLog("bind: {}", strerror(errno));
		return 0;
    }

    socket_nonblock(s);

    if (protocol == SOCK_STREAM)
    {
		PyLog("SYSTEM: BINDING TCP PORT ON [{}] (fd {})", port, s);
		listen(s, SOMAXCONN);
    }
    else
		PyLog("SYSTEM: BINDING UDP PORT ON [{}] (fd {})", port, s);

    return s;
}

int socket_tcp_bind(const char* ip, int port)
{
    return socket_bind(ip, port, SOCK_STREAM);
}

int socket_udp_bind(const char* ip, int port)
{
    return socket_bind(ip, port, SOCK_DGRAM);
}

void socket_close(socket_t s)
{
#ifdef __WIN32__
    closesocket(s);
#else
    close(s);
#endif
}

socket_t socket_accept(socket_t s, struct sockaddr_in *peer)
{
    socket_t desc;
    socklen_t i;

    i = sizeof(*peer);

    if ((desc = accept(s, (struct sockaddr*) peer, &i)) == -1)
    {
		SysLog("accept: {} (fd {})", strerror(errno), s);
		return -1;
    }

    if (desc >= 65500)
    {
		SysLog("SOCKET FD 65500 LIMIT! {}", desc);
		socket_close(s);
		return -1;
    }

    socket_nonblock(desc);
    socket_lingeroff(desc);
	
    return (desc);
}

socket_t socket_connect(const char* host, uint16_t port)
{
    socket_t            s = 0;
    struct sockaddr_in  server_addr;
    int                 rslt;

	/* Initialize the socket address structure */
    memset(&server_addr, 0, sizeof(server_addr));

    if (isdigit(*host))
		server_addr.sin_addr.s_addr = inet_addr(host);
    else
    {
		struct hostent *hp;

		if ((hp = gethostbyname(host)) == NULL)
		{
			SysLog("socket_connect(): can not connect to {}:{}", host, port);
			return -1;
		}

		memcpy((char* ) &server_addr.sin_addr, hp->h_addr, sizeof(server_addr.sin_addr));
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
	perror("socket");
	return -1;
    }

    socket_keepalive(s);
    socket_sndbuf(s, 233016);
    socket_rcvbuf(s, 233016);
    socket_timeout(s, 10, 0);
    socket_lingeron(s);

	/* connection request */
    if ((rslt = connect(s, (struct sockaddr*) &server_addr, sizeof(server_addr))) < 0)
    {
	socket_close(s);

#ifdef __WIN32__
	switch (WSAGetLastError())
#else
	    switch (rslt)
#endif
	    {
#ifdef __WIN32__
			case WSAETIMEDOUT:
#else
			case EINTR:
#endif
				SysLog("HOST {}:{} connection timeout.", host, port);
				break;
#ifdef __WIN32__
			case WSAECONNREFUSED:
#else
			case ECONNREFUSED:
#endif
				SysLog("HOST {}:{} port is not opened. connection refused.", host, port);
				break;
#ifdef __WIN32__
			case WSAENETUNREACH:
#else
			case ENETUNREACH:
#endif
                SysLog("HOST {}:{} is not reachable from this host.", host, port);
				break;

			default:
                SysLog("HOST {}:{}, could not connect.", host, port);
				break;
	    }

	perror("connect");
	return (-1);
    }

    return (s);
}

#ifndef __WIN32__

#ifndef O_NONBLOCK
#define O_NONBLOCK O_NDELAY
#endif

void socket_nonblock(socket_t s)
{
    int flags;

    flags = fcntl(s, F_GETFL, 0);
    flags |= O_NONBLOCK;

    if (fcntl(s, F_SETFL, flags) < 0) 
    {
		sys_err("fcntl: nonblock: %s", strerror(errno));
		return;
    }
}

void socket_block(socket_t s)
{
    int flags;

    flags = fcntl(s, F_GETFL, 0);
    flags &= ~O_NONBLOCK;

    if (fcntl(s, F_SETFL, flags) < 0)
    {
		sys_err("fcntl: nonblock: %s", strerror(errno));
		return;
    }
}
#else
void socket_nonblock(socket_t s)
{
    unsigned long val = 1;
    ioctlsocket(s, FIONBIO, &val);
}

void socket_block(socket_t s)
{
    unsigned long val = 0;
    ioctlsocket(s, FIONBIO, &val);
}
#endif

void socket_dontroute(socket_t s)
{
    int set;

    if (setsockopt(s, SOL_SOCKET, SO_DONTROUTE, (const char*) &set, sizeof(int)) < 0)
    {
        SysLog("setsockopt: dontroute: {}", strerror(errno));
		socket_close(s);
		return;
    }
}

void socket_lingeroff(socket_t s)
{
#ifdef __WIN32__
    int linger;
    linger = 0;
#else
    struct linger linger;

    linger.l_onoff	= 0;
    linger.l_linger	= 0;
#endif
    if (setsockopt(s, SOL_SOCKET, SO_LINGER, (const char*) &linger, sizeof(linger)) < 0)
    {
        SysLog("setsockopt: linger: {}", strerror(errno));
		socket_close(s);
		return;
    }
}

void socket_lingeron(socket_t s)
{
#ifdef __WIN32__
    int linger;
    linger = 0;
#else
    struct linger linger;

    linger.l_onoff	= 1;
    linger.l_linger	= 0;
#endif
    if (setsockopt(s, SOL_SOCKET, SO_LINGER, (const char*) &linger, sizeof(linger)) < 0)
    {
		SysLog("setsockopt: linger: {}", strerror(errno));
		socket_close(s);
		return;
    }
}

void socket_rcvbuf(socket_t s, unsigned int opt)
{
    socklen_t optlen;

    optlen = sizeof(opt);

    if (setsockopt(s, SOL_SOCKET, SO_RCVBUF, (const char*) &opt, optlen) < 0)
    {
        SysLog("setsockopt: rcvbuf: {}", strerror(errno));
		socket_close(s);
		return;
    }

    opt         = 0;
    optlen      = sizeof(opt);

    if (getsockopt(s, SOL_SOCKET, SO_RCVBUF, (char*) &opt, &optlen) < 0)
    {
        SysLog("getsockopt: rcvbuf: {}", strerror(errno));
		socket_close(s);
		return;
    }

    TraceLog("SYSTEM: {}: receive buffer changed to {}", s, opt);
}

void socket_sndbuf(socket_t s, unsigned int opt)
{
    socklen_t optlen;

    optlen = sizeof(opt);

    if (setsockopt(s, SOL_SOCKET, SO_SNDBUF, (const char*) &opt, optlen) < 0)
    {
        SysLog("setsockopt: sndbuf: {}", strerror(errno));
		return;
    }

    opt         = 0;
    optlen      = sizeof(opt);

    if (getsockopt(s, SOL_SOCKET, SO_SNDBUF, (char*) &opt, &optlen) < 0)
    {
        SysLog("getsockopt: sndbuf: {}", strerror(errno));
		return;
    }

    TraceLog("SYSTEM: %d: send buffer changed to {}", s, opt);
}

// sec : seconds, usec : microseconds
void socket_timeout(socket_t s, long sec, long usec)
{
#ifndef __WIN32__
    struct timeval      rcvopt;
    struct timeval      sndopt;
    socklen_t		optlen = sizeof(rcvopt);

    rcvopt.tv_sec = sndopt.tv_sec = sec;
    rcvopt.tv_usec = sndopt.tv_usec = usec;
#else
    socklen_t		rcvopt, sndopt;
    socklen_t		optlen = sizeof(rcvopt);
    sndopt = rcvopt = (sec * 1000) + (usec / 1000);
#endif
    if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const char*) &rcvopt, optlen) < 0)
    {
        SysLog("setsockopt: timeout: {}", strerror(errno));
		socket_close(s);
		return;
    }

    if (getsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (char*) &rcvopt, &optlen) < 0)
    {
        SysLog("getsockopt: timeout: {}", strerror(errno));
		socket_close(s);
		return;
    }

    optlen = sizeof(sndopt);

    if (setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, (const char*) &sndopt, optlen) < 0)
    {
        SysLog("setsockopt: timeout: {}", strerror(errno));
		socket_close(s);
		return;
    }

    if (getsockopt(s, SOL_SOCKET, SO_SNDTIMEO, (char*) &sndopt, &optlen) < 0)
    {
        SysLog("getsockopt: timeout: {}", strerror(errno));
		socket_close(s);
		return;
    }

#ifndef __WIN32__
    TraceLog("SYSTEM: {}: TIMEOUT RCV: {}.{}, SND: {}.{}", s, rcvopt.tv_sec, rcvopt.tv_usec, sndopt.tv_sec, sndopt.tv_usec);
#endif
}

void socket_reuse(socket_t s)
{
    int opt = 1;

    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const char*) &opt, sizeof(opt)) < 0)
    {
        SysLog("setsockopt: reuse: {}", strerror(errno));
		socket_close(s);
		return;
    }
}

void socket_keepalive(socket_t s)
{
    int opt = 1;

    if (setsockopt(s, SOL_SOCKET, SO_KEEPALIVE, (const char*) &opt, sizeof(opt)) < 0)
    {
		perror("setsockopt: keepalive");
		socket_close(s);
		return;
    }
}
