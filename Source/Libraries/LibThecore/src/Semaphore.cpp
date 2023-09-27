#include "stdafx.h"
#include "Semaphore.h"

#ifndef __WIN32__
CSemaphore::CSemaphore() : m_hSem(nullptr)
{
	Initialize();
}

CSemaphore::~CSemaphore()
{
	Destroy();
}

int32_t CSemaphore::Initialize()
{
	Clear();

	m_hSem = new sem_t;

	if (sem_init(m_hSem, 0, 0) == -1)
	{
		perror("sem_init");
		return false;
	}

	return true;
}

void CSemaphore::Destroy()
{
	Clear();
}

void CSemaphore::Clear()
{
	if (m_hSem)
	{
		sem_destroy(m_hSem);
		delete m_hSem;
	}

	m_hSem = nullptr;
}

int32_t CSemaphore::Wait()
{
	if (!m_hSem)
		return true;

	int32_t re = sem_wait(m_hSem);

	if (re == -1)
	{
		if (errno == EINTR)
			return Wait();

		perror("sem_wait");
		return false;
	}

	return true;
}

int32_t CSemaphore::Release(int32_t count)
{
	if (!m_hSem)
		return false;

	for (int32_t i = 0; i < count; ++i)
		sem_post(m_hSem);

	return true;
}
#else
CSemaphore::CSemaphore() : m_hSem(nullptr)
{
	Initialize();
}

CSemaphore::~CSemaphore()
{
	Destroy();
}

bool CSemaphore::Initialize()
{
	Clear();

	m_hSem = ::CreateSemaphore(NULL, 0, 32, nullptr);

	if (m_hSem == nullptr)
	{
		return false;
	}

	return true;
}

void CSemaphore::Destroy()
{
	Clear();
}

void CSemaphore::Clear()
{
	if (m_hSem == nullptr)
	{
		return;
	}
	::CloseHandle(m_hSem);
	m_hSem = nullptr;
}

bool CSemaphore::Wait()
{
	if (!m_hSem)
		return true;

	uint32_t dwWaitResult = ::WaitForSingleObject(m_hSem, INFINITE);

	switch (dwWaitResult) 
	{
		case WAIT_OBJECT_0:
			return true;
		default:
			break;
	}
	return false;
}

bool CSemaphore::Release(int32_t count)
{
	if (!m_hSem)
		return false;

	::ReleaseSemaphore(m_hSem, count, nullptr);

	return true;
}
#endif
