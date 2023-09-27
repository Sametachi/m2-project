#pragma once

#include <storm/memory/NewAllocator.hpp>

class SimpleApp
{
    public:
        SimpleApp();
        ~SimpleApp();
    
    protected:
        storm::NewAllocator m_allocator;
};

#define SIMPLE_APPLICATION(ImplName) \
extern "C" int main(int argc, const char** argv) \
{ \
	return ImplName().Run(argc, argv); \
}
