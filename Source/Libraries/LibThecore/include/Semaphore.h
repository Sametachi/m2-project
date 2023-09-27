#pragma once

#ifndef __WIN32__
#include <semaphore.h>
#endif

class CSemaphore
{
public:
	CSemaphore();
	~CSemaphore();

	bool Initialize();
	void Clear();
	void Destroy();

	bool Wait();
	bool Release(int32_t count = 1);

private:
#ifdef _WIN32
	void* m_hSem;
#else
	sem_t m_hSem;
#endif
};
