// 
// 
// 

#include "RingPufferIndex.h"

RingPufferIndexClass::RingPufferIndexClass(int size)
{
	maxSize = size ;
	nextWriteIdx = 0;
	nextReadIdx = 0;
}

int RingPufferIndexClass::getNextWriteIndex()
{
	int idx = nextWriteIdx;
	nextWriteIdx = inc(nextWriteIdx);
	if (nextWriteIdx == nextReadIdx)
	{	// full
		nextReadIdx = inc(nextReadIdx);
		// todo: Error melden?
	}
	return idx;
}

int RingPufferIndexClass::getNextReadIndex()
{
	if (nextWriteIdx == nextReadIdx)
	{	// empty
		return -1;
	}

	int idx = nextReadIdx;
	nextReadIdx = inc(nextReadIdx);
	return idx;
}

int RingPufferIndexClass::inc(int idx)
{
	return  (idx + 1) % maxSize;
}