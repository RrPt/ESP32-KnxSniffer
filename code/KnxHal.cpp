// KnxHal.cpp
//
// KNX Hardware Absttraction Layer 
//
// (c) Rainer Petzoldt

#include "KnxHal.h"

RingPufferIndexClass* KnxHalClass::ringBuffer;
char KnxHalClass::tele[MAXTELEGRAMCOUNT][MAXTELEGRAMSIZE];
int KnxHalClass::teleLen[MAXTELEGRAMCOUNT];
long KnxHalClass::teleTime[MAXTELEGRAMCOUNT];
long KnxHalClass::teleNo[MAXTELEGRAMCOUNT];
int KnxHalClass::actualTelIdx;
long KnxHalClass::startTime;

byte KnxHalClass::telegram[MAXTELEGRAMSIZE];

int KnxHalClass::bitcounter;
int KnxHalClass::bytecounter;
volatile int KnxHalClass::telegramcounter;

int KnxHalClass::totalBitCounter;

byte KnxHalClass::inPinNo;
byte KnxHalClass::outPinNo;
int KnxHalClass::parity;
bool KnxHalClass::error;
volatile int KnxHalClass::errorCounter;
hw_timer_t * KnxHalClass::timer = NULL;

void KnxHalClass::init(byte inPinNo_, byte outPinNo_)
{
	ringBuffer = new RingPufferIndexClass(MAXTELEGRAMCOUNT);
	inPinNo = inPinNo_;
	outPinNo = outPinNo_;
	bitcounter = 0;
	bytecounter = 0;
	totalBitCounter = 0;
	telegramcounter = 0;
	for (size_t i = 0; i < MAXTELEGRAMSIZE; i++)
	{
		telegram[i] = 0;
	}
	parity = 0;
	error = false;
	errorCounter = 0;
	Serial.println("KnxHalClass.init started");
	pinMode(inPinNo, INPUT_PULLUP);
	pinMode(DEBUGPORT1, OUTPUT);
	attachInterrupt(inPinNo, isrPulse, FALLING);
	// init timer
	timer = timerBegin(0, 4, true);  
	timerAttachInterrupt(timer, &isrTimer, true);
	timerAlarmWrite(timer, 2081, true);  // Damit 104,05 us  passt empirisch am besten
	timerAlarmEnable(timer);

}

void KnxHalClass::StartTimer(uint64_t us)
{
	timerWrite(timer, us);
	timerAlarmEnable(timer);
}

void KnxHalClass::StopTimer()
{
	timerAlarmDisable(timer);
}

void IRAM_ATTR KnxHalClass::isrPulse() 
{
	ulong startZeit = micros();
	while (micros() - startZeit < 13);
	StartTimer(0);  // erster irq nach 15us
	startTime = millis();
	bitcounter = 1;
	bytecounter = 0;
	totalBitCounter = 1;
	error = false;
	parity = 0;
	detachInterrupt(inPinNo);  // die bits macht der Timer irq, hier stört der ioIrq
}

uint8_t toggle;

void wait(int len)
{
	ulong startZeit = micros();
	while (micros() - startZeit < 2*len);
}

void IRAM_ATTR KnxHalClass::isrTimer() 
{
	int bit = digitalRead(inPinNo);
	wait(bit*10);
	if (bitcounter!=0 && bitcounter < 9)
	{
		telegram[bytecounter] |= bit << (bitcounter - 1);
		if (bit) parity = !parity;
	}
	
	if (bitcounter == 9)
	{	// parity prüfen
		if (bit != parity)
		{
			//error = true;      // Errorcounter hochzählen aber Telegramm trotzdem anzeigen, ist ja schließlich ein Sniffer
			errorCounter++;
		}
	}
	
	if (bitcounter == 0)
	{ // neues Startbit
		digitalWrite(DEBUGPORT1, true);

		if (bit == 1)
		{	// Telegrammende wenn kein Startbit
            if (!error)
			{   // Telegramm vollständig empfangen
				// Telegramm in Ausgabepuffer kopieren
				int idx = ringBuffer->getNextWriteIndex();
				memcpy(&tele[idx], &telegram, MAXTELEGRAMSIZE);
				teleLen[idx] = bytecounter;
				teleTime[idx] = startTime;
				teleNo[idx] = telegramcounter++;
			}
			bytecounter = 0;   
			bitcounter = 0;
			totalBitCounter = 0;
			telegram[bytecounter] = 0;
			StopTimer();	// Timer aus, wird erst wieder bei neuem Telegramm gestartet
			attachInterrupt(inPinNo, isrPulse, FALLING);  // auf neues Telegramm warten (fallende Flanke auf dem Bus)
		}
	}
	 

	if (bitcounter > 11)
	{
		digitalWrite(DEBUGPORT1, false);
		bitcounter = 0;
		bytecounter++;  
		parity = 0;
		telegram[bytecounter] = 0;

		if (bytecounter >= MAXTELEGRAMSIZE)
		{
			errorCounter++;
			bytecounter = 0;    // vermeidet Pufferüberlauf Telegramm verwerfen   
			telegram[bytecounter] = 0;
			bitcounter = 0;
			totalBitCounter = 0;
			StopTimer();
			attachInterrupt(inPinNo, isrPulse, FALLING);  // auf neues Telegramm warten
		}
	}
	else
	{
		totalBitCounter++;
		bitcounter++;
	}

	digitalWrite(DEBUGPORT1, false);
}

