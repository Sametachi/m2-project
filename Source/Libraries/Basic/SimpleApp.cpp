#include <Basic/SimpleApp.hpp>

SimpleApp::SimpleApp(): m_allocator()
{
	storm::SetDefaultAllocator(&m_allocator);
}

SimpleApp::~SimpleApp()
{
	storm::SetDefaultAllocator(nullptr);
}
