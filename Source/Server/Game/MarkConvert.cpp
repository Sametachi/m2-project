#include "stdafx.h"
#include "MarkManager.h"

#ifdef __WIN32__
#include <direct.h>
#endif

#define OLD_MARK_INDEX_FILENAME "guild_mark.idx"
#define OLD_MARK_DATA_FILENAME "guild_mark.tga"

static Pixel * LoadOldGuildMarkImageFile()
{
	FILE * fp = fopen(OLD_MARK_DATA_FILENAME, "rb");

	if (!fp)
	{
		SysLog("cannot open {}", OLD_MARK_INDEX_FILENAME);
		return NULL;
	}

	int32_t dataSize = 512 * 512 * sizeof(Pixel);
	Pixel * dataPtr = (Pixel *) malloc(dataSize);

	fread(dataPtr, dataSize, 1, fp);

	fclose(fp);

	return dataPtr;
}

bool GuildMarkConvert(const std::vector<uint32_t> & vecGuildID)
{
#ifndef __WIN32__
	mkdir("mark", S_IRWXU);
#else
	_mkdir("mark");
#endif

#ifndef __WIN32__
	if (0 != access(OLD_MARK_INDEX_FILENAME, F_OK))
#else
	if (0 != _access(OLD_MARK_INDEX_FILENAME, 0))
#endif
		return true;

	FILE* fp = fopen(OLD_MARK_INDEX_FILENAME, "r");

	if (!fp)
		return false;

	Pixel * oldImagePtr = LoadOldGuildMarkImageFile();

	if (!oldImagePtr)
	{
		fclose(fp);
		return false;
	}

	/*
	// guild_mark.tga is not an actual targa file, but a raw file of 512 * 512 * 4 size.
	// Make it a real targa file for visual verification.
	CGuildMarkImage* pImage = new CGuildMarkImage;
	pImage->Build("guild_mark_real.tga");
	pImage->Load("guild_mark_real.tga");
	pImage->PutData(0, 0, 512, 512, oldImagePtr);
	pImage->Save("guild_mark_real.tga");
	*/
	PyLog("Guild Mark Converting Start.");

	char line[256];
	uint32_t guild_id;
	uint32_t mark_id;
	Pixel mark[SGuildMark::SIZE];

	while (fgets(line, sizeof(line)-1, fp))
	{
		sscanf(line, "%u %u", &guild_id, &mark_id);

		if (find(vecGuildID.begin(), vecGuildID.end(), guild_id) == vecGuildID.end())
		{
			PyLog("  skipping guild ID {}", guild_id);
			continue;
		}

		uint row = mark_id / 32;
		uint col = mark_id % 32;

		if (row >= 42)
		{
			SysLog("invalid mark_id {}", mark_id);
			continue;
		}

		uint sx = col * 16;
		uint sy = row * 12;

		Pixel * src = oldImagePtr + sy * 512 + sx;
		Pixel * dst = mark;

		for (int32_t y = 0; y != SGuildMark::HEIGHT; ++y)
		{
			for (int32_t x = 0; x != SGuildMark::WIDTH; ++x)
				*(dst++) = *(src+x);

			src += 512;
		}

		CGuildMarkManager::GetInstance()->SaveMark(guild_id, (uint8_t*) mark);
		line[0] = '\0';
	}

	free(oldImagePtr);
	fclose(fp);

#ifndef __WIN32__
	system("mv -f guild_mark.idx guild_mark.idx.removable");
	system("mv -f guild_mark.tga guild_mark.tga.removable");
#else
	system("move /Y guild_mark.idx guild_mark.idx.removable");
	system("move /Y guild_mark.tga guild_mark.tga.removable");
#endif

	PyLog("Guild Mark Converting Complete.");

	return true;
}

