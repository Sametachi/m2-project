#pragma once
#include <functional>

//#ifdef _DEBUG
bool CreateConsoleWindow();

class ScopedConsoleCloseHandler
{
	public:
		ScopedConsoleCloseHandler(std::function<void()> h);
		~ScopedConsoleCloseHandler();

		void operator()();

	private:
		std::function<void()> m_handler;
};
//#endif
