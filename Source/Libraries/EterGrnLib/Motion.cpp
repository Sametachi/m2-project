#include "StdAfx.h"
#include "Motion.h"

CGrannyMotion::CGrannyMotion()
{
	Initialize();
}

CGrannyMotion::~CGrannyMotion()
{
	Destroy();
}

bool CGrannyMotion::IsEmpty()
{
	return m_pgrnAni ? false : true;
}

void CGrannyMotion::Destroy()
{
	Initialize();
}

void CGrannyMotion::Initialize()
{
	m_pgrnAni = nullptr;
}

bool CGrannyMotion::BindGrannyAnimation(granny_animation* pgrnAni)
{
	assert(IsEmpty());

	m_pgrnAni = pgrnAni;
	return true;
}

granny_animation* CGrannyMotion::GetGrannyAnimationPointer() const
{
	return m_pgrnAni;
}

const char* CGrannyMotion::GetName() const
{
	return m_pgrnAni->Name;
}

float CGrannyMotion::GetDuration() const
{
	return m_pgrnAni->Duration;
}

void CGrannyMotion::GetTextTrack(const char* c_szTextTrackName, int32_t* pCount, float* pArray) const
{
	if (m_pgrnAni->TrackGroupCount != 1)
	{
		// That sucks..
	}

	granny_track_group* pTrack = m_pgrnAni->TrackGroups[0];

	for (int32_t i = 0; i < pTrack->TextTrackCount; ++i)
	{
		granny_text_track& rTextTrack = pTrack->TextTracks[i];
		for (int32_t j = 0; j < rTextTrack.EntryCount; ++j)
		{
			if (!_stricmp(c_szTextTrackName, rTextTrack.Entries[j].Text))
			{
				pArray[(*pCount)++] = rTextTrack.Entries[j].TimeStamp;
			}
		}
	}
}

