#include "stdafx.h"
#include "PeerBase.h"

CPeerBase::CPeerBase() : m_fd(INVALID_SOCKET), m_BytesRemain(0), m_outBuffer(nullptr), m_inBuffer(nullptr)
{
	memset(m_szHost, 0, sizeof(m_szHost));
}

CPeerBase::~CPeerBase()
{
	Destroy();
}

void CPeerBase::Disconnect()
{
	if (m_fd != INVALID_SOCKET)
	{
		fdwatch_del_fd(m_fdWatcher, m_fd);

		socket_close(m_fd);
		m_fd = INVALID_SOCKET;
	}
}

void CPeerBase::Destroy()
{
	Disconnect();

	if (m_outBuffer)
	{
		buffer_delete(m_outBuffer);
		m_outBuffer = nullptr;
	}

	if (m_inBuffer)
	{
		buffer_delete(m_inBuffer);
		m_inBuffer = nullptr;
	}
}

bool CPeerBase::Accept(socket_t fd_accept)
{
	struct sockaddr_in pPeer;

	if ((m_fd = socket_accept(fd_accept, &pPeer)) == INVALID_SOCKET)
	{
		Destroy();
		return false;
	} 

	// socket_block(m_fd);
	socket_sndbuf(m_fd, 233016);
	socket_rcvbuf(m_fd, 233016);

	strlcpy(m_szHost, inet_ntoa(pPeer.sin_addr), sizeof(m_szHost));
	m_outBuffer = buffer_new(DEFAULT_PACKET_BUFFER_SIZE);
	m_inBuffer = buffer_new(MAX_INPUT_LEN);

	if (!m_outBuffer || !m_inBuffer)
	{
		Destroy();
		return false;
	}

	fdwatch_add_fd(m_fdWatcher, m_fd, this, FDW_READ, false);

	OnAccept();
	PyLog("ACCEPT FROM {}", inet_ntoa(pPeer.sin_addr));
	return true;
}

bool CPeerBase::Connect(const char* host, uint16_t port)
{
	strlcpy(m_szHost, host, sizeof(m_szHost));

	if ((m_fd = socket_connect(host, port)) == INVALID_SOCKET)
		return false;

	m_outBuffer = buffer_new(DEFAULT_PACKET_BUFFER_SIZE);

	if (!m_outBuffer)
	{
		Destroy();
		return false;
	}

	fdwatch_add_fd(m_fdWatcher, m_fd, this, FDW_READ, false);

	OnConnect();
	return true;
}

void CPeerBase::Close()
{
	OnClose();
}

void CPeerBase::EncodeBYTE(uint8_t b)
{
	if (!m_outBuffer)
	{
		SysLog("Not ready to write");
		return;
	}

	buffer_write(m_outBuffer, &b, 1);
	fdwatch_add_fd(m_fdWatcher, m_fd, this, FDW_WRITE, true);
}

void CPeerBase::EncodeWORD(uint16_t w)
{
	if (!m_outBuffer)
	{
		SysLog("Not ready to write");
		return;
	}

	buffer_write(m_outBuffer, &w, 2);
	fdwatch_add_fd(m_fdWatcher, m_fd, this, FDW_WRITE, true);
}

void CPeerBase::EncodeDWORD(uint32_t dw)
{
	if (!m_outBuffer)
	{
		SysLog("Not ready to write");
		return;
	}

	buffer_write(m_outBuffer, &dw, 4);
	fdwatch_add_fd(m_fdWatcher, m_fd, this, FDW_WRITE, true);
}

void CPeerBase::Encode(const void* data, uint32_t size)
{
	if (!m_outBuffer)
	{
		SysLog("Not ready to write");
		return;
	}

	buffer_write(m_outBuffer, data, size);
	fdwatch_add_fd(m_fdWatcher, m_fd, this, FDW_WRITE, true);
}

int32_t CPeerBase::Recv()
{
	if (!m_inBuffer)
	{
		SysLog("input buffer nil");
		return -1;
	}

	buffer_adjust_size(m_inBuffer, MAX_INPUT_LEN >> 2);
	int32_t bytes_to_read = buffer_has_space(m_inBuffer);
	ssize_t bytes_read = socket_read(m_fd, (char*) buffer_write_peek(m_inBuffer), bytes_to_read);

	if (bytes_read < 0)
	{
		SysLog("socket_read failed {}", strerror(errno));
		return -1;
	}
	else if (bytes_read == 0)
		return 0;

	buffer_write_proceed(m_inBuffer, bytes_read);
	m_BytesRemain = buffer_size(m_inBuffer);
	return 1;
}

void CPeerBase::RecvEnd(int32_t proceed_bytes)
{
	buffer_read_proceed(m_inBuffer, proceed_bytes);
	m_BytesRemain = buffer_size(m_inBuffer);
}

int32_t CPeerBase::GetRecvLength()
{
	return m_BytesRemain;
}

const void* CPeerBase::GetRecvBuffer()
{
	return buffer_read_peek(m_inBuffer);
}

int32_t CPeerBase::GetSendLength()
{
	return buffer_size(m_outBuffer);
}

int32_t CPeerBase::Send()
{
	if (buffer_size(m_outBuffer) <= 0)
		return 0;

	int32_t iBufferLeft = fdwatch_get_buffer_size(m_fdWatcher, m_fd);
	int32_t iBytesToWrite = MIN(iBufferLeft, buffer_size(m_outBuffer));

	if (iBytesToWrite == 0)
		return 0;

	int32_t result = socket_write(m_fd, (const char*) buffer_read_peek(m_outBuffer), iBytesToWrite);

	if (result == 0)
	{
		buffer_read_proceed(m_outBuffer, iBytesToWrite);

		if (buffer_size(m_outBuffer) != 0)
			fdwatch_add_fd(m_fdWatcher, m_fd, this, FDW_WRITE, true);
	}

	return (result);
}
