//***********************************************************
//	a3gs.h -- IEM 3G Sheild for Arduino Control Library
//
//	History:
//		R1.0 2012/10/06  1st Release for IEM 3G Shiled(Ver1.0)
//		R1.1 2012/10/14  2nd Release(Operate by few memories)
//						 (modify sendSMS(),onSMSReceived(),httpGET(),httpPOST(),tweet(),connectTCP())
//		R1.2 2012/10/28  Bug fix (httpGET(), httpPOST(), tweet(), availableSMS() and redSMS())
//		R1.3 2013/01/01  Support "https" in httpGET() and httpPost()
//		R1.4 2013/04/16  Bug fix (discardUntil() has never return)
//			-->> a3gsMAX_TWEET_LENGTH set as 60 (2013.6.3)
//		R2.0 2013/07/10  Change interface of TCP/IP and discardUntil(), some bugs fix
//
//	Author:
//		3G Shield Alliance and Atushi Daikoku
//
//	Notes:
//		Lower compatible with Arduino GSM/GPRS Shield library.
//		Notices as bellow:
//		- Use SoftwareSerial library (RxD is D4, TxD is D5).
//		- Use Interrupt 0(D2) for SMS arrival notification.
//		- Use D6 and D7 are used for Power control.
//		If you want to change UART baudrate then define "a3gsBAUDRATE"
//		symbol before #include "a3gs.h" statement.
//***********************************************************

#ifndef _A3GS_H_
#define _A3GS_H_  1

#include <Arduino.h>
#include <SoftwareSerial.h>

/*
	Define constants
*/
//	for compatibility of GSM.h
#define	ctrlz					26		// Ascii character for ctr+z. End of a SMS.
#define	cr						13		// Ascii character for carriage return. 
#define	lf						10		// Ascii character for line feed. 

//	Basic constants
#define	a3gsBAUDRATE			4800	// Default UART baudrate if "a3gsBAUDRATE" is undefined
#define	a3gsDATE_SIZE			11		// date space size ("YYYY/MM/DD\0" format, used in getTime(), included '\0')
#define	a3gsTIME_SIZE			9		// time space size ("HH:MM:SS\0" format(hour is 24h-way), used in getTime(), included '\0')
#define	a3gsCS_ASCII			0		// SMS Text Char Code: ASCII (used in sendSMS())
#define	a3gsCS_UNICODE			1		// SMS Text Char Code: UNICODE(Little Endian) (used in sendSMS())
#define	a3gsIMEI_SIZE			16		// imei space size ("99..9\0" format, used in getIMEI(), included '\0')
#define	a3gsDEFAULT_PORT		0		// Default port number(for httpGET() and httpPOST())

//	Return values in general
#define	a3gsSUCCESS				0
#define	a3gsERROR				(-1)

//	Maximum lengths
#define	a3gsMAX_VERSION_LENGTH	5		// Maximum bytes of Version number(used in getVersion())
#define	a3gsMAX_SMS_LENGTH		100		// Maximum bytes of SMS message(used in sendSMS())
#define	a3gsMAX_MSN_LENGTH		11		// Maximum bytes of Phone Number in Japan(used in readSMS())
#define	a3gsMAX_PROFILE_NUMBER	16		// Profile number is 1..a3gsMAX_PROFILE_NUMBER
#define	a3gsMAX_HOST_LENGTH		128		// Maximum length of host name
#define	a3gsMAX_DATA_LENGTH		1024	// Maximum length of data(at read/write)
#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
	// Mega and Mega 2560/ADK
#define	a3gsMAX_URL_LENGTH		256		// Maximum length of URL(used in httpGET() and httpPOST())
#define	a3gsMAX_RESULT_LENGTH	1024	// Maximum length of result(used in httpGET() and httpPOST())
#define	a3gsMAX_HEADER_LENGTH	512		// Maximum length of header(used in httpPOST())
#define	a3gsMAX_BODY_LENGTH		1024	// Maximum length of header(used in httpPOST())
#define	a3gsMAX_TWEET_LENGTH	140		// Maximum length of Tweet message(used in tweet())
#else // UNO, Lenoard, Pro, Fio..
#define	a3gsMAX_URL_LENGTH		128		// Maximum length of URL(used in httpGET() and httpPOST())
#define	a3gsMAX_RESULT_LENGTH	192 //@ 256		// Maximum length of result(used in httpGET() and httpPOST())
#define	a3gsMAX_HEADER_LENGTH	192 //@ 256		// Maximum length of header(used in httpPOST())
#define	a3gsMAX_BODY_LENGTH		128 //@ 256		// Maximum length of header(used in httpPOST())
#define	a3gsMAX_TWEET_LENGTH	60 //@ 100		// Maximum length of Tweet message(used in tweet())
#endif

//	Return values of getService()
#define	a3gsSRV_NO				0		// Out of service
#define	a3gsSRV_PS				1		// Data(packet) only
#define	a3gsSRV_CS				2		// Voice only
#define	a3gsSRV_BOTH			3		// Data and voice both

//	Method of positioning by getPosition()
#define	a3gsMPBASED				0		// GPS + AGPS
#define	a3gsMPASSISTED			1		// AGPS
#define	a3gsMPSTANDALONE		2		// GPS only

/*
	Declare class
*/
class A3GS
{
  public:
	enum A3GS_st_e { ERROR, IDLE, READY, TCPCONNECTEDCLIENT };

	// compatible methods with Arduino GSM/GPRS Shield library
	int getStatus() { return _status; };
	int begin(char* pin = 0);
	int begin(char* pin, uint32_t baudrate);
	int end(void);
	int restart(char* pin = 0);
	int start(char* pin = 0);
	int shutdown(void);
		//-- This library do not use "pin" parameter, so ignore it.
	int getIMEI(char* imei);
	int sendSMS(const char* to, const char* msg, int encode = a3gsCS_ASCII);
	boolean availableSMS();
	int readSMS(char* msg, int msglength, char* number, int nlength);
	int connectTCP(const char* server, int port);
	int disconnectTCP();
	int write(uint8_t c);
	int write(const char* str);
	int write(const uint8_t* buffer, size_t sz);
	int read(char* result, int resultlength);
	int read(void);		//@R2.0 Change
		//-- This library do not support tcp/ip server functions.
	int httpGET(const char* server, uint16_t port, const char* path, char* result, int resultlength, boolean ssled = false);
	int httpPOST(const char* server, uint16_t port, const char* path, const char* header, const char* body, char* result, int* resultlength, boolean ssled = false);
		//-- httpPOST() is not compatible parameters with Arduino GSM/GPRS Shield library
	int tweet(const char* token, const char* msg);
		//-- tweet() sends a message to twitter with the text "msg"and the token "token". 
		//-- Get the token from http://arduino-tweet.appspot.com/

	//-- Extended methods of IEM 3G Shield --//
	int onSMSReceived(void (*handler)(void));
	int getLocation(int method, char* latitude, char* longitude);
	int getServices(int& status);
	int getRSSI(int& rssi);
	int getTime(char* date, char* time);
	int getTime2(uint32_t& seconds);
	int getVersion(char *version);
	int setDefaultProfile(int profileNum);
	int getDefaultProfile(int* profileNum);
	int setBaudrate(uint32_t baudrate);
	int setLED(boolean sw);

  private:
	int _status;		// for Compatible with GSM.h
	void sendCommand(const char* cmd);
	void sendData(const char* data);
	void discardUntil(const char match);
	int getResult(char *buf, int *len, uint32_t timeout);
	static void handleINT0(void);
};

extern	A3GS	a3gs;		// An A3GS object

#endif // _A3GS_H_

