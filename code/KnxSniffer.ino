/*
	ESP32 Knx Sniffer   by Rainer Petzoldt  R.Petzoldt@web.de
 */

//  bisher nur lesen von Bus, kein schreiben

#include "RingPufferIndex.h"
#include "KnxHal.h"
#include <WiFi.h>
// for disable Brownout
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

WiFiServer server(23);  // TelenetServer
WiFiServer serverK(8023);  // TelenetServer im TRX Format  zum Import in ETS (log mit Putty, dann im ETS Gruppentelegramme einlesen)
WiFiClient client;
WiFiClient clientK;

const char* ssid = "***";
const char* password = "***";

bool printed = false;
int lastTelegramCounter = 0;

void setup() 
{
	Serial.begin(115200);
	Serial.println("KnxSniffer started");

	// Disable Brownout
	int bor = READ_PERI_REG(RTC_CNTL_BROWN_OUT_REG);
	Serial.printf("brownoutReg=%X\n", bor);
	WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0x0); //disable brownout detector
	bor = READ_PERI_REG(RTC_CNTL_BROWN_OUT_REG);
	Serial.printf("brownoutReg=%X\n", bor);

	WiFi.begin(ssid, password);
	Serial.println("Connecting Wifi ");
	for (int loops = 10; loops > 0; loops--) {
		if (WiFi.status() == WL_CONNECTED) {
			Serial.println("");
			Serial.print("WiFi connected ");
			Serial.print("IP address: ");
			Serial.println(WiFi.localIP());
			break;
		}
		else {
			Serial.println(loops);
			if (loops % 3 == 0) WiFi.begin(ssid, password);
			delay(500);
		}
	}
	if (WiFi.status() != WL_CONNECTED) {
		Serial.println("WiFi connect failed");
		delay(1000);
		ESP.restart();
	}
	server.begin();
	serverK.begin();
	KnxHalClass::init(34, 0);
}

void loop() 
{
	int idx = KnxHalClass::ringBuffer->getNextReadIndex();
	if (idx != -1)
	{	// Telegramm vorhanden
		const int buflen = 200;
		char out[buflen];
		char outK[buflen];
		int len = KnxHalClass::teleLen[idx];
		int pos = snprintf(out,buflen,"t=%s  No=%3d len=%2d idx=%1d KnxData=", toTime(KnxHalClass::teleTime[idx]).c_str(), KnxHalClass::teleNo[idx], len, idx);
		int posK = snprintf(outK, buflen, "%s\t29 ", toTime(KnxHalClass::teleTime[idx]).c_str());
		//Serial.printf("out=%s\n", out);
		for (size_t i = 0; i < 12 ; i++)
		{
			if (i < KnxHalClass::teleLen[idx])
			{
				pos += snprintf(out + pos, buflen-pos, "%02X ", KnxHalClass::tele[idx][i]);
				posK += snprintf(outK + posK, buflen-posK, "%02X ", KnxHalClass::tele[idx][i]);
			}
			else pos += snprintf(out + pos, buflen, "   ");
		}
		if (len == 1 && KnxHalClass::tele[idx][0] == 0xCC) pos += snprintf(out + pos, buflen, "ACK\n");
		else if (len > 4)
		{
			pos += snprintf(out + pos, buflen, "%s", quellAdr(KnxHalClass::tele[idx]).c_str());
			pos += snprintf(out + pos, buflen, " -> %s", zielAdr(KnxHalClass::tele[idx]).c_str());
			int dataLen = KnxHalClass::tele[idx][5] & 0x0F;
			pos += snprintf(out + pos, buflen, " DATA(%d)=", dataLen);
			if (dataLen == 1)
			{
				pos += snprintf(out + pos, buflen, " %d", KnxHalClass::tele[idx][7] & 0x7F);
			}
			else if (dataLen == 2)
			{
				pos += snprintf(out + pos, buflen, " %02X %02X", KnxHalClass::tele[idx][7]& 0x7F, KnxHalClass::tele[idx][8]);
			}
			else if (dataLen == 3)
			{	
				pos += snprintf(out + pos, buflen, " %02X %02X %02X", KnxHalClass::tele[idx][7] & 0x7F, KnxHalClass::tele[idx][8], KnxHalClass::tele[idx][9]);
				pos += snprintf(out + pos, buflen, " %8.3f",toFloat(KnxHalClass::tele[idx][8], KnxHalClass::tele[idx][9]));
			}
			else if (dataLen == 4)
			{	
				pos += snprintf(out + pos, buflen, " %02X %02X %02X %02X", KnxHalClass::tele[idx][7] & 0x7F, KnxHalClass::tele[idx][8], KnxHalClass::tele[idx][9], KnxHalClass::tele[idx][10]);
				pos += snprintf(out + pos, buflen, "  %d:%02d:%02d", KnxHalClass::tele[idx][8], KnxHalClass::tele[idx][9], KnxHalClass::tele[idx][10]);
			}
			pos += snprintf(out + pos, buflen, "\n");
		}
		else
		{
			pos += snprintf(out + pos, buflen, "?\n");
		}

		Serial.printf("%s", out);


		if (client.connected())
		{
			client.printf("%s", out);
		}
		if (clientK.connected() & ((KnxHalClass::teleLen[idx]!=1 | KnxHalClass::tele[idx][0]!=0xCC)))
		{	// keine ACK
			int checkSum = 12;  // todo berechnen
			clientK.printf("%s %02d\r\n", outK, checkSum);
		}
	}
	// Telnet Connection
	if (server.hasClient())
	{
		if (client.connected())
		{
			Serial.println("Telnet Connection rejected");
			server.available().stop();
		}
		else
		{
			client = server.available();
			Serial.print("Telent Connection accepted from ");
			Serial.println(client.remoteIP() );
			client.printf("Verbunden mit ");
			client.println(client.localIP());
		}
	}
	// Server2
	if (serverK.hasClient())
	{
		if (clientK.connected())
		{
			Serial.println("Telnet Connection rejected");
			serverK.available().stop();
		}
		else
		{
			clientK = serverK.available();
			Serial.print("Telent Connection accepted from ");
			Serial.println(clientK.remoteIP());
			clientK.printf("Verbunden mit ");
			clientK.println(clientK.localIP());
		}
	}
}

String quellAdr(char* tele)
{
	char buf[10];
	snprintf(buf, 10, "%d.%d.%-3d", tele[1] >> 4, tele[1] & 0x0F, tele[2]);
	return String(buf);
}

String zielAdr(char* tele)
{
	char buf[10];
	if (tele[5] & 0x80 == 0)
	{
		snprintf(buf, 10, "%d.%d.%-3d", tele[3] >> 4, tele[3] & 0x0F, tele[4]);
	}
	else
	{
		snprintf(buf, 10, "%d/%d/%-3d", tele[3] >> 3, tele[3] & 0x07, tele[4]);
	}
	return String(buf);
}

float toFloat(byte l, byte h)
{
	short e = (short)((l>> 3) & 0x0f);
	short m = (short)(((l & 0x07) + ((l >> 4) & 0x8)) * 256 + h);
	if ((l & 0x80) == 0x80) m = (short)(m - 0x1000);

	return (0.01f * m * (1 << e));

	//return float(1.0*a*b);  // todo: nur test
}

String toTime(long totalMs)
{
	unsigned long runMillis = totalMs % 1000;
	unsigned long allSeconds = totalMs / 1000;
	int runHours = allSeconds / 3600;
	int secsRemaining = allSeconds % 3600;
	int runMinutes = secsRemaining / 60;
	int runSeconds = secsRemaining % 60;

	char buf[21];
	sprintf(buf, "%02d:%02d:%02d.%03d", runHours, runMinutes, runSeconds,runMillis);
	return String(buf);
}