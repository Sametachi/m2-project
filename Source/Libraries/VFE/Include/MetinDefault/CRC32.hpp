#pragma once
#include <string>
#include <windows.h>

uint32_t GetCRC32(const char* buffer, size_t count);
uint32_t GetCaseCRC32(const char* buf, size_t len);
uint32_t get_crc32(HANDLE hFile);
uint32_t GetFileCRC32(const char* c_szFileName);
uint32_t GetFileSize(const char* c_szFileName);
uint32_t ComputeCrc32(uint32_t previousCrc32, const void* data, uint32_t length);
