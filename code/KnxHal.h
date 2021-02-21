// KnxHal.h
//
// KNX Hardware Absttraction Layer 
//
// (c) Rainer Petzoldt

#ifndef _KNXHAL_h
#define _KNXHAL_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

#include "RingPufferIndex.h"

#define DEBUGPORT1 26   // für Debugausgabe
#define MAXTELEGRAMSIZE 30
#define MAXTELEGRAMCOUNT 10

class KnxHalClass
{
public:
	 static RingPufferIndexClass* ringBuffer;
	 static int actualTelIdx;
	 static long startTime;

public:
	 static char tele[MAXTELEGRAMCOUNT][MAXTELEGRAMSIZE];
	 static int teleLen[MAXTELEGRAMCOUNT];
	 static long teleTime[MAXTELEGRAMCOUNT];
	 static long teleNo[MAXTELEGRAMCOUNT];

 public:
	 static volatile int errorCounter;
	 static volatile int telegramcounter;

private:
	 static byte telegram[MAXTELEGRAMSIZE];
	 static int bitcounter ;
	 static int bytecounter;
	 static int totalBitCounter;
	 static int parity;
	 static bool error;
	 static hw_timer_t * timer;

 private:
	 static byte inPinNo;
	 static byte outPinNo;

 public:
	static void init(byte inPinNo, byte outPinNo);
	static void StartTimer(uint64_t us);
	static void StopTimer();

private:
	static void IRAM_ATTR isrPulse();
	static void IRAM_ATTR isrTimer();
};

#endif

