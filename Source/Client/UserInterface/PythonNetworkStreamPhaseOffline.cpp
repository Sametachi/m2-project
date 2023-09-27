#include "StdAfx.h"
#include "PythonNetworkStream.h"
#include "PythonApplication.h"

void CPythonNetworkStream::OffLinePhase()
{
	TPacketHeader header;

	if (!CheckPacket(&header))
		return;

	switch (header)
	{
		case HEADER_GC_PHASE:
			if (RecvPhasePacket())
				return;
			break;
	}

	RecvErrorPacket(header);
}