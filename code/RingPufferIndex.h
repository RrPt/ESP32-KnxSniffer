// RingPufferIndex.h

#ifndef _RINGPUFFERINDEX_h
#define _RINGPUFFERINDEX_h



class RingPufferIndexClass
{
protected:
	int maxSize = 9;
	int nextWriteIdx = 0;
	int nextReadIdx = 0;

	int inc(int idx);

public:
	RingPufferIndexClass(int size);
	int getNextWriteIndex();
	int getNextReadIndex();
};

#endif

