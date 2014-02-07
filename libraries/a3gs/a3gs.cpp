//***********************************************************
//	a3gs.cpp -- IEM 3G Sheild for Arduino Control Library
//
//	History:
//		R1.0 2012/10/06  1st Release for IEM 3G Shiled(Ver1.0)
//		R1.1 2012/10/14  2nd Release(Operate by few memories)
//						 (modify sendSMS(),onSMSReceived(),httpGET(),httpPOST(),tweet(),connectTCP())
//		R1.2 2012/10/28  Bug fix (httpGET(), httpPOST(), tweet(), availableSMS() and redSMS())
//		R1.3 2013/01/01  Support "https" in httpGET() and httpPost() [if gw3g version is above 1.1]
//		R1.4 2013/04/16  Bug fix (discardUntil() has never return)
//		R2.0 2013/07/12  Change interface of TCP/IP and discardUntil()
//						 some bugs fix(httpPOST, httpGET, read and getResult)
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
//		If You use Leonard, Mega or ADK(Mega2560) then you must change IEM_RXD_PIN and IEM_TXD_PIN for suitable.
//***********************************************************

//#define	DEBUG				1		// Define if you want to debug

#include "Arduino.h"
#include "a3gs.h"

// Macros
#ifdef DEBUG
#  define DEBUG_PRINT(m,v)	{ Serial.print("** "); Serial.print((m)); Serial.print(":"); Serial.println((v)); }
#else
#  define DEBUG_PRINT(m,v)	// do nothing
#endif
	// Constants for pins
#define	IEM_INT_PIN			2		// D2(INT0)
#define	IEM_RXD_PIN			4		// D4(For example,If you use Leonardo or Mega etc. then change to D10..)
#define	IEM_TXD_PIN			5		// D5(For example,If you use Leonardo or Mega etc. then change to D11..)
#define	IEM_POWER_PIN		6		// D6
#define	IEM_REGULATOR_PIN	7		// D7
	// Constants for getTime2()
#define	SECS_PER_MIN 		(60UL)
#define	SECS_PER_HOUR		(3600UL)
#define	SECS_PER_DAY		(SECS_PER_HOUR * 24UL)
#define	DAYS_PER_WEEK		(7UL)
#define	SECS_YR_2000		(946684800UL)		// the time at the start of y2k
	// Leap year calulator expects year argument as years offset from 1970
#define	LEAP_YEAR(Y)		(((1970+Y)>0) && !((1970+Y)%4) && (((1970+Y)%100) || !((1970+Y)%400)))
	// Command execution timeout values
#define	TIMEOUT_LOCAL		5000	// Timeout value of local functions [mS]
#define	TIMEOUT_NETWORK		35000	// Timeout value of communication functions [mS]
#define	TIMEOUT_GPS			180000	// Timeout value og GPS locationing [mS]
	// Misc.
#define	ISDIGIT(c)			((c) >= '0' && (c) <= '9')
	// Interrupt number for SMS
#define	INTNO_SMS			0		// Use INT0
	// IEM Version
#define	MIN_IEM_VERSION		1.0		// A necessary minimum IEM version of this library

// Define an A3GS(Arduino 3G Shield) Object Here.
A3GS	a3gs;

// Global variables
#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
static char gWorkBuffer[256];		// Buffer for working(Mega..)
#else
static char gWorkBuffer[150];		// Buffer for working(UNO..)
#endif
static SoftwareSerial iemSerial(IEM_RXD_PIN, IEM_TXD_PIN);		// Serial for IEM Interface
static void (*SMShandler)(void);								// ISR for SMS Notification


//***************************
//	begin()
//
//	@description
//		Begin to use 3G shield function
//	@return value
//		0 .. OK(at least, Packet Switch or Circuit Switch is availabled)
//		otherwise .. NG
//	@param
//		pin : Not used
//	@note
//		
//***************************
int A3GS::begin(char* pin)
{
	char	version[a3gsMAX_VERSION_LENGTH+1];
//--
	DEBUG_PRINT(">begin()", a3gsBAUDRATE);

	// Begin SoftwareSerial
	iemSerial.begin(a3gsBAUDRATE);

	// Initilalize global variables and detach INT0
	_status = IDLE;
	SMShandler = NULL;
	detachInterrupt(INTNO_SMS);

	// Get iem version and check it
	if (getVersion(version) != 0)
		return 1;	// NG -- Can't get version

	if (atof(version) < MIN_IEM_VERSION) {
		DEBUG_PRINT(">getVersion()", "IEM version is old");
		return 2;	// NG -- IEM Version is old
	}

	return a3gsSUCCESS;  // OK
}

//***************************
//	begin()
//
//	@description
//		Begin to use 3G shield function
//	@return value
//		0 .. OK(at least, Packet Switch or Circuit Switch is availabled)
//		otherwise .. NG
//	@param
//		pin : Not used
//		baudrate : baudrate to begin (2400/4800/9600/19200/38400/57600/115200)
//	@note
//		This function use to specify non-default baudrate.
//***************************
int A3GS::begin(char* pin, uint32_t baudrate)
{
	char	version[a3gsMAX_VERSION_LENGTH+1];
//--
	DEBUG_PRINT(">begin()", baudrate);

	// Begin SoftwareSerial
	iemSerial.begin(baudrate);

	// Initilalize global variables and detach INT0
	_status = IDLE;
	SMShandler = NULL;
	detachInterrupt(INTNO_SMS);

	// Get iem version and check it
	if (getVersion(version) != 0)
		return 1;	// NG -- Can't get version

	if (atof(version) < MIN_IEM_VERSION) {
		DEBUG_PRINT(">getVersion()", "IEM version is old");
		return 2;	// NG -- IEM Version is old
	}

	return a3gsSUCCESS;  // OK
}

//***************************
//	end()
//
//	@description
//		End to use 3G shield function
//	@return value
//		0 .. always
//	@param
//		none
//	@note
//		re-enable serial communication, call a3gs.begin()
//***************************
int A3GS::end(void)
{
	// Clear interrupt handler
	if (SMShandler != NULL) {
		detachInterrupt(INTNO_SMS);
		SMShandler = NULL;
	}

	// End SoftwareSerial
	iemSerial.end();

	return a3gsSUCCESS;	// OK
}

//***************************
//	restart()
//
//	@description
//		Restart 3G shield and clear SMS handler
//	@return value
//		0 .. always
//	@param
//		pin : Not used
//	@note
//		Reboot time is about 40 Sec.
//***************************
int A3GS::restart(char* pin)
{
	// Clear interrupt handler
	if (SMShandler != NULL) {
		detachInterrupt(INTNO_SMS);
		SMShandler = NULL;
	}

	sendCommand("$YE");		// Send "Reset IEM" Command

	return a3gsSUCCESS;	// OK
}

//***************************
//	start()
//
//	@description
//		Power on 3G shield and wait for ready to use
//	@return value
//		0 .. always
//	@param
//		pin : Not used
//	@note
//		Start up time is about 40 Sec.
//***************************
int A3GS::start(char* pin)
{
	pinMode(IEM_INT_PIN, INPUT);		// for SMS notification interrupt
	pinMode(IEM_REGULATOR_PIN, OUTPUT);
	pinMode(IEM_POWER_PIN, OUTPUT);

	digitalWrite(IEM_REGULATOR_PIN, HIGH);	// Regulator turn on
	delay(1000);		// Wait a moment
	digitalWrite(IEM_POWER_PIN, LOW);
	delay(2000);		// Make low STATUS_PB_WALK -> Turn on IEM
	digitalWrite(IEM_POWER_PIN, HIGH);

	DEBUG_PRINT(">start()", "Turn on and wait for a moment..");

	delay(35000);	// Wait for ready IEM

	DEBUG_PRINT(">start()", "IEM is available now.");

	return a3gsSUCCESS;	// OK
}

//***************************
//	shutdown()
//
//	@description
//		Power off 3G shield
//	@return value
//		0 .. always
//	@param
//		none
//	@note
//		Shutdown time is about 15 Sec.
//***************************
int A3GS::shutdown(void)
{
	sendCommand("$YE");		// Send "Reset IEM" Command

	delay(15000);	// Wait a moment

	digitalWrite(IEM_REGULATOR_PIN, LOW);	// Regulator turn off

	return a3gsSUCCESS;	// OK
}

//***************************
//	getService
//
//	@description
//		Get network service status
//	@return value
//		0 .. Succeeded, otherwise .. Failed
//	@param
//		status .. [OUT] SRV* (see a3gs.h)
//	@note
//		
//***************************
int A3GS::getServices(int& status)
{
	char responses[12];		// Format "$YS=OK 9" or "$YS=NG errno"
	int  length = sizeof(responses);
//--
	sendCommand("$YS");	// Send "Get Services" command

	if (getResult(responses, &length, TIMEOUT_LOCAL))
		return 1;	// NG -- maybe timeout

	DEBUG_PRINT(">getServices()", responses);

	// parse response
	switch (responses[7]) {
		case '0' :		// NO_SRV
			status = a3gsSRV_NO;
			break;
		case '1' :		// PS_ONLY
			status = a3gsSRV_PS;
			break;
		case '2' :		// CS_ONLY
			status = a3gsSRV_CS;
			break;
		case '3' :		// BOTH
			status = a3gsSRV_BOTH;
			break;
		default : 		// bug
			return 1;		// NG -- Can't get service
	}

	return a3gsSUCCESS;		// OK
}

//***************************
//	getIMEI()
//
//	@description
//		Get IEM's IMEI
//	@return value
//		0 .. Succeeded
//		otherwise .. Failed
//	@param
//		imei : [OUT] IMEI (a3gsIMEI_SIZE bytes space requred)
//	@note
//		
//***************************
int A3GS::getIMEI(char* imei)
{
	char responses[a3gsIMEI_SIZE+10];  // Format "$YI=OK 99..99" or "$YI=NG errno"
	int  length = sizeof(responses);
//--
	sendCommand("$YI");		// Send "Get IMEI" command

	if (getResult(responses, &length, TIMEOUT_LOCAL))
		return 1;  // NG -- maybe timeout

	DEBUG_PRINT(">getIMEI()", responses);

	// parse response
	if (strncmp(responses, "$YI=OK", 6))
		return 1;	// NG -- Can't get IMEI

	strncpy(imei, responses+7, a3gsIMEI_SIZE);
	imei[a3gsIMEI_SIZE] = '\0';

	return a3gsSUCCESS;	// OK
}

//***************************
//	setBaudrate
//
//	@description
//		Set UART baudrate
//	@return value
//		0 .. OK
//		1 .. NG
//	@param
//		baudrate : baudrate to set (2400/4800/9600/19200/38400/57600/115200)
//	@note
//		New baudrate will be validated after reset IEM
//***************************
int A3GS::setBaudrate(uint32_t baudrate)
{
	char command[13];
	char responses[12];  // Format "$YB=OK" or "$YB=NG errno"
	int  length = sizeof(responses);
//--
	switch (baudrate) {
		case 2400 :
		case 4800 :
		case 9600 :
		case 19200 :
		case 38400 :
		case 57600 :
		case 115200 :
			break;
		default :
			return 1;	// NG -- Bad parameter
	}

	// Make command and send it
	sprintf(command, "$YB %lu", baudrate);
	sendCommand(command);	// Send "Set Baudrate" command

	if (getResult(responses, &length, TIMEOUT_LOCAL))
		return 1;  // NG -- maybe timeout

	DEBUG_PRINT(">setBaudrate()", responses);

	// parse response
	if (strncmp(responses, "$YB=OK", 6))
		return 1;	// NG -- Can't set baudrate

	return a3gsSUCCESS;	// OK
}

//***************************
//	setLED
//
//	@description
//		Set LED On/Off
//	@return value
//		0 .. OK
//		1 .. NG
//	@param
//		sw : true is On, false is Off
//	@note
//		There is LED on 3G shield board(named LED1)
//***************************
int A3GS::setLED(boolean sw)
{
	char command[10];
	char responses[12];  // Format "$YL=OK" or "$YL=NG errno"
	int  length = sizeof(responses);
//--
	// Make command and send it
	sprintf(command, "$YL %d", (sw ? 1 : 0));
	sendCommand(command);	// Send "Set LED" command

	if (getResult(responses, &length, TIMEOUT_LOCAL))
		return 1;  // NG -- maybe timeout

	DEBUG_PRINT(">setLED()", responses);

	// parse response
	if (strncmp(responses, "$YL=OK", 6))
		return 1;	// NG -- Can't set led

	return a3gsSUCCESS;	// OK
}

//***************************
//	sendSMS
//
//	@description
//		send SMS
//	@return value
//		0 .. OK
//		otherwise .. NG
//	@param
//		to : MSN(10 digits)
//		msg : message string(ASCII or UNICODE)
//		encode : message's char set(default is ASCII)
//***************************
int A3GS::sendSMS(const char* to, const char* msg, int encode)
{
	char responses[20];  // FORMAT: "$SS=OK" or "$SS=NG errno"
	int  length = sizeof(responses);
//--
	// Check parameter sizes
	if (strlen(msg) > a3gsMAX_SMS_LENGTH)
		return 1;	// NG - too long message
	if (strlen(to) > a3gsMAX_MSN_LENGTH)
		return 1;	// NG - too long msn

	// Send comamnd little by little
	sendData("$SS ");
	sendData(to);
	sendData(" \"");
	sendData(msg);
	sendData("\" ");
	if (encode == a3gsCS_UNICODE)
		sendCommand("UNICODE");
	else
		sendCommand("ASCII");

	DEBUG_PRINT(">sendSMS", to);

	if (getResult(responses, &length, TIMEOUT_NETWORK))
		return 1;	// NG -- maybe timeout

	DEBUG_PRINT(">sendSMS()", responses);

	if (strncmp(responses, "$SS=OK", 6))
		return 1;	// NG -- send error

	return a3gsSUCCESS;	// OK
}

//***************************
//	availableSMS
//
//	@description
//		Check SMS is available or not
//	@return value
//		true .. Available
//		false .. Not Available
//	@param
//		none
//	@note
//		
//***************************
boolean A3GS::availableSMS()
{
  char responses[20];		// FORMAT: "$SC=OK n" or "$SC=NG errno"
  int  length = sizeof(responses);
//--
	sendCommand("$SC");	// Send "Check SMS" command

	if (getResult(responses, &length, TIMEOUT_LOCAL))
		return false;	// NG -- maybe timeout

	DEBUG_PRINT(">availableSMS()", responses);

	if (strncmp(responses, "$SC=OK 1", 8));
		return false;	// Not available

	return true;	// Available
}

//***************************
//	readSMS
//
//	@description
//		Read received SMS
//	@return value
//		0 .. OK
//		otherwise .. NG
//	@param
//		msg : [OUT] read message string
//		msglength : msg length(max a3gsMAX_SMS_LENGTH)
//		number : [OUT] MSN
//		nlength : MSN length(max a3gsMAX_MSN_LENGTH)
//	@note
//		Use gWorkBuffer and destory it
//***************************
int A3GS::readSMS(char* msg, int msglength, char* number, int nlength)
{
	int	length = sizeof(gWorkBuffer);
	char *cp;
//--
	sendCommand("$SR");		// Send "Read SMS" command

	if (getResult(gWorkBuffer, &length, TIMEOUT_LOCAL))
		return 1;	// NG -- maybe timeout

	if (! strncmp(gWorkBuffer, "$SR=NG", 6))
		return 1;	// NG -- No SMS

	// Parse number(msn)
	for (cp = gWorkBuffer+7; *cp != '\0'; cp++) {
		while (ISDIGIT(*cp) && nlength-- > 1)
			*number++ = *cp++;
		*number = '\0';
		DEBUG_PRINT(">readSMS() msn", number);
		break;
	}

	// Parse message
	for ( ; *cp != '\0'; cp++) {
		if (*cp != '\"')
			continue;	// Skip until "
		cp++;	// skip "
		while (*cp != '\0' && *cp != '\"' && msglength-- > 1)
			*msg++ = *cp++;
		*msg = '\0';
		DEBUG_PRINT(">readSMS() msg", msg);
		break;
	}

	return a3gsSUCCESS;	// OK
}

//***************************
//	onSMSReceived
//
//	@description
//		Set call back function when SMS was receieved
//	@return value
//		0 .. OK(always)
//	@param
//		handler : Call back function(Set) or NULL(Cancel)
//	@note
//		This function use attachInterrupt() and detachInterrupt() with interrupt number 0(D2).
//		Interrupt mode is "LOW".
//		It is necessary to call onSMSReceived() each time and to set up handler to use continuously.
//***************************
int A3GS::onSMSReceived(void (*handler)(void))
{
	if (handler == NULL)
		detachInterrupt(INTNO_SMS);
	else
		attachInterrupt(INTNO_SMS, handleINT0, LOW);

	SMShandler = handler;

	return a3gsSUCCESS;	// OK
}
    
//***************************
//	httpGET
//
//	@description
//		Request HTTP/GET to server and port
//
//	@return value
//		0 .. OK
//		otherwise .. NG
//	@param
//		server : server name(ex. "www,google.com")
//		port : port number(ex. 80)
//		path : path(ex. service/index.html)
//		result : [OUT] responce raw data(no escaped, '\0' terminated)
//		resultlength ; "result" size(max MAX_RESULT_LENGTH)
//		ssled : if true then use https(SSL), otherwise use http
//	@note
//		This function is synchronized.
//***************************
int A3GS::httpGET(const char* server, uint16_t port, const char* path, char* result, int resultlength, boolean ssled)
{
	int	length = sizeof(gWorkBuffer);
	int	nbytes;
	int	c;		//@R2.0 Change
//--
	// Send command line little by little
	if (ssled) {
		if (strlen(server) + strlen(path) + 8 > a3gsMAX_URL_LENGTH)
			return 1;  // NG -- Too long url

		sendData("$WG https://");
		sendData(server);
		if (port != a3gsDEFAULT_PORT) {
			char ports[6];
			sendData(":");
			sprintf(ports, "%u", port);
			sendData(ports);
		}
	}
	else {
		if (strlen(server) + strlen(path) + 7 > a3gsMAX_URL_LENGTH)
			return 1;  // NG -- Too long url

		sendData("$WG http://");
		sendData(server);
		if (port != a3gsDEFAULT_PORT) {
			char ports[6];
			sendData(":");
			sprintf(ports, "%u", port);
			sendData(ports);
		}
	}
	sendCommand(path);	// Go HTTP/GET request

	DEBUG_PRINT(">httpGET()", "REQ");

	if (getResult(gWorkBuffer, &length, TIMEOUT_NETWORK))
		return 1;	// NG -- maybe timeout

	DEBUG_PRINT(">httpGET()", gWorkBuffer);
		// result's format: $WG=OK nbytes "response"

	// Parse response
	if (strncmp(gWorkBuffer, "$WG=OK", 6))
		return 1;	// NG -- Can't get response

	nbytes = atoi(gWorkBuffer + 7);
	DEBUG_PRINT(">httpGET() nbytes", nbytes);

	// Copy response body into "result" and set "resultlength"
	uint32_t	startTime = millis();
	for (int n = 0; n < nbytes; n++) {
		while (iemSerial.available() <= 0) {
			// Wait until valid response
			if (millis() - startTime >= TIMEOUT_NETWORK)
				return 2;	// NG -- Timeout
		}
		c = iemSerial.read();
		if (n < resultlength - 1)
			result[n] = c;
	}
	discardUntil('\n');	// discard last '\n' -- @R2.0 Change
	if (nbytes > resultlength - 1)			//@R2.0 Change
		result[resultlength - 1] = '\0';
	else
		result[nbytes] = '\0';

	return a3gsSUCCESS;		// OK
}

//***************************
//	httpPOST
//
//	@discription
//		Request HTTP/POST to server and port
//
//	@return value
//		0 .. OK
//		otherwise .. NG
//  @param
//		server : server name(ex. "www,google.com")
//		port : port number(ex. 80)
//		path : path(ex. service/index.html)
//		header : header(without Content-Length, nul terminate, max MAX_HEADER_LENGTH)
//		body : request body(nul terminate, max MAX_BODY_LENGTH)
//		result : [OUT] responce raw data(no escaped)
//		resultlength ; [OUT] "result" size(max MAX_RESULT_LENGTH)
//		ssled : if true then use https(SSL), otherwise use http
//	@note
//		This function is synchronized.
//***************************
int A3GS::httpPOST(const char* server, uint16_t port, const char* path, const char *header, const char *body, char* result, int* resultlength, boolean ssled)
{
	int	length = sizeof(gWorkBuffer);
	int	nbytes;
	int	c;		//@R2.0 Change
//--
	if (strlen(body) > a3gsMAX_BODY_LENGTH)
		return 1;	// NG -- too long body
	if (header != NULL && strlen(header) > a3gsMAX_HEADER_LENGTH)
		return 1;	// NG -- too long header

	// Send command line little by little
	if (ssled) {
		if (strlen(server) + strlen(path) + 8 > a3gsMAX_URL_LENGTH)
			return 1;  // NG -- Too long url

		sendData("$WP https://");
		sendData(server);
		if (port != a3gsDEFAULT_PORT) {
			char ports[6];
			sendData(":");
			sprintf(ports, "%u", port);
			sendData(ports);
		}
	}
	else {
		if (strlen(server) + strlen(path) + 7 > a3gsMAX_URL_LENGTH)
			return 1;  // NG -- Too long url

		sendData("$WP http://");
		sendData(server);
		if (port != a3gsDEFAULT_PORT) {
			char ports[6];
			sendData(":");
			sprintf(ports, "%u", port);
			sendData(ports);
		}
	}

	//if (path[0] != '/')
	//	sendData("/");		// for compatiblity old version
	sendData(path);
	//sendData(body);
	sendData("\"");
	//sendData(body);
	//sendData("     \"");
	//sendData(header);
    //sendData(" \"");
	//if (header != NULL && *header != '\0') {
	//	sendData("\" \"");

	//}

	sendCommand("\"");	// Go command

	DEBUG_PRINT(">httpPOST()", "REQ");

	if (getResult(gWorkBuffer, &length, TIMEOUT_NETWORK))
		return 1;	// NG -- maybe timeout

	DEBUG_PRINT(">httpPOST()", gWorkBuffer);
		// result's format: $WP=OK nbytes or $WP=NG errno

	// Parse response
	if (strncmp(gWorkBuffer, "$WP=OK", 6))
		return 1;	// NG -- Can't post or get response

	nbytes = atoi(gWorkBuffer + 7);
	DEBUG_PRINT(">httpPOST() nbytes", nbytes);

	// Copy response body into "result" and set "resultlength"
	uint32_t	startTime = millis();
	for (int n = 0; n < nbytes; n++) {
		while (iemSerial.available() <= 0) {
			// Wait until valid response
			if (millis() - startTime >= TIMEOUT_NETWORK)
				return 2;	// NG -- Timeout
		}
		c = iemSerial.read();
		if (n < *resultlength - 1)
			result[n] = c;
	}
	discardUntil('\n');	// discard last '\n' -- @R2.0 Change
	if (nbytes > *resultlength - 1)			//@R2.0 Change
		result[*resultlength - 1] = '\0';
	else {
		result[nbytes] = '\0';
		*resultlength = nbytes;
	}

	return a3gsSUCCESS;		// OK
}

//***************************
//	tweet
//
//	@description
//		Tweet a message
//
//	@return value
//		0 .. OK
//		otherwise .. NG
//	@param
//		token .. token from http://arduino-tweet.appspot.com/
//		msg .. message to tweet(in UTF-8)
//	@note
//		This function use cloud service on "arduino-tweet.appspot.com".
//		Notice from this service:
//		- The library uses this site as a proxy server for OAuth stuff. Your tweet may not be applied during maintenance of this site.
//		- Please avoid sending more than 1 request per minute not to overload the server.
//		- Twitter seems to reject repeated tweets with the same contenet (returns error 403).
//		Use gWorkBuffer and destory it

//***************************
int A3GS::tweet(const char* token, const char* msg)
{
	int	resultlength = sizeof(gWorkBuffer);
//--
	if (strlen(msg) > a3gsMAX_TWEET_LENGTH)
		return 8;  // Too long message. (@@ check by number of characters, not bytes)

	sprintf(gWorkBuffer, "token=%s&status=%s", token, msg);

	// request POST to http://arduino-tweet.appspot.com
	return httpPOST("arduino-tweet.appspot.com", 80, "update", NULL, gWorkBuffer, gWorkBuffer, &resultlength);
}

//***************************
//	getTime
//
//	@description
//		Get current date and time
//	@return value
//		0 .. OK
//		otherwise .. NG
//	@param
//		date : [OUT] current date(JST) ("YYYY/MM/DD" format)
//		time : [OUT] current time(JST) ("HH:MM:SS" format)
//	@note
//		
//***************************
int A3GS::getTime(char date[], char time[])
{
	char	responses[28];  // Format "$YT=OK 2012/03/18 13:28:25"
	int	length = sizeof(responses);
//--
	sendCommand("$YT");		// Send "Get Time" command

	if (getResult(responses, &length, TIMEOUT_LOCAL))
		return 1;	// NG -- maybe timeout

	DEBUG_PRINT(">getTime()", responses);

	if (strncmp(responses, "$YT=OK", 6))
		return 1;	// NG -- Can't get time

	// Parse response
	strncpy(date, responses + 7, 10);
	date[10] = '\0';

	strncpy(time, responses + 18, 8);
	time[8] = '\0';

	return a3gsSUCCESS;	// OK
}

//***************************
//	getTime2
//
//	@description
//		Get the current time as seconds since Jan 1 1970
//	@return value
//		0 .. OK
//		otherwise .. NG
// 	@param
//		seconds : [OUT] Current seconds since Jan 1 1970(JST)
//	@noyte
//		
//***************************
int A3GS::getTime2(uint32_t& seconds)
{
	static  const uint8_t monthDays[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
	char date[a3gsDATE_SIZE];
	char time[a3gsTIME_SIZE];
	uint8_t year, month, day, hour, minute, second;
	uint32_t t;   // total seconds from 1970/01/01 00:00:00
//--
	// Get Current Time
	if (getTime(date, time) != 0)
		return 1;	// NG -- can't get time, Failed.

	year = atoi(date) - 1970;
	month = atoi(date+5);
	day = atoi(date+8);
	hour = atoi(time);
	minute = atoi(time+3);
	second = atoi(time+6);

	// Seconds from 1970 till 1 jan 00:00:00 of the given year
	t = year * (SECS_PER_DAY * 365);
	for (int y = 0; y < year; y++) {
		if (LEAP_YEAR(y))
			t += SECS_PER_DAY;	// add extra days for leap years
	}

	// Add days for this year, months start from 1
	for (int m = 1; m < month; m++) {
		if ((m == 2) && LEAP_YEAR(year))
			t += SECS_PER_DAY * 29;
		else
			t += SECS_PER_DAY * monthDays[m-1];  //monthDay array starts from 0
	}
	t += (day - 1) * SECS_PER_DAY;
	t += hour * SECS_PER_HOUR;
	t += minute * SECS_PER_MIN;
	t += second;

	seconds = t;

	return a3gsSUCCESS;	// OK
}

//***************************
//	getRSSI
//
//	@description
//		Get 3G RSSI
//	@return value
//		0 .. OK
//		1 .. NG
//	@param
//		rssi : [OUT] rssi (dBm)
//	@note
//		
//***************************
int A3GS::getRSSI(int& rssi)
{
	char	responses[13];  // Format "$YR=OK -999"
	int	length = sizeof(responses);
//--
	sendCommand("$YR");	// Send "Get RSSI" command

	if (getResult(responses, &length, TIMEOUT_LOCAL))
		return 1;	// NG -- maybe timeout

	DEBUG_PRINT(">getRSSI()", responses);

	// Parse response
	rssi = atoi(responses + 7);

	return a3gsSUCCESS;	// OK
}

//***************************
//	getVersion
//
//	@description
//		Get Version
//	@return value
//		0 .. OK
//		1 .. NG
//	@param
//		version : [OUT] version ("X.YY")
//	@note
//		
//***************************
int A3GS::getVersion(char *version)
{
	char	responses[13];  // Format "$YV=OK 9.99"
	int	length = sizeof(responses);
//--
	sendCommand("$YV");	// Send "Get Version" command

	if (getResult(responses, &length, TIMEOUT_LOCAL))
		return 1;	// NG -- maybe timeout

	DEBUG_PRINT(">getVersion()", responses);

	// Copy version
	strcpy(version, responses + 7);

	return a3gsSUCCESS;	// OK
}

//***************************
//	getLocation
//
//	@description
//		get Current Location from GPS and Network
//
//	@return value
//		0 .. OK
//		1 .. NG(Bad parameters)
//		otherwise .. NG(Can't locate position..)
//	@param
//		method .. method of locate(a3gsM)
//		latitude .. [OUT] latitude
//		longitude .. [OUT] longitude
//	@note
//		10-180 Sec. required.
//***************************
int A3GS::getLocation(int method, char* latitude, char* longitude)
{
	char *cp, responses[40];	// Format "$LG=OK lat lng"
	int	length = sizeof(responses);
//--
	*latitude = '\0';
	*longitude = '\0';

	// Make command and send it
	switch (method) {
		case a3gsMPBASED :
			sendCommand("$LG MSBASED");  // Default is method=MSBASED
			break;
		case a3gsMPASSISTED :
			sendCommand("$LG MSASSISTED");
			break;
		case a3gsMPSTANDALONE :
			sendCommand("$LG STANDALONE");
			break;
		default :
			return 1;  // bad method
	}

	if (getResult(responses, &length, TIMEOUT_GPS))
		return 1;	// NG -- maybe timeout

	DEBUG_PRINT(">getLocation()", responses);

	// parse response
	if (! strncmp(responses, "$LG=NG", 6))
		return 1;	// NG --  Can't locate

	// Parse lattitude (assume enough space to store latitude)
	for (cp = responses+7; *cp != '\0'; cp++) {
		while (ISDIGIT(*cp) || *cp == '.')
			*latitude++ = *cp++;
		*latitude = '\0';
		DEBUG_PRINT(">getLocation() lat", latitude);
		break;
	}

	// Skip spaces
	while (*cp != '\0' && ! ISDIGIT(*cp))
		cp++;

	// Parse longitude (assume enough space to store longitude)
	for ( ; *cp != '\0'; cp++) {
		while (ISDIGIT(*cp) || *cp == '.')
			*longitude++ = *cp++;
		*longitude = '\0';
		DEBUG_PRINT(">getLocation() lng", longitude);
		break;
	}

	return a3gsSUCCESS;	// OK
}

//***************************
//	setDefaultProfile
//
//	@description
//		Set default profile(APN) number
//	@return value
//		0 .. OK
//		1 .. NG(Bad parameters)
//	@param
//		profileNum : profile number(1..a3gsMAX_PROFILE_NUMBER)
//	@note
//		
//***************************
int A3GS::setDefaultProfile(int profileNum)
{
	char	responses[20];	// Format "$PS=OK"
	char	commands[20];	// Format "$PS 99"
	int	length = sizeof(responses);
//--
	if (profileNum < 1 || a3gsMAX_PROFILE_NUMBER < profileNum)
		return 1;	// NG -- Bad parameter

	sprintf(commands, "$PS %d", profileNum);
	sendCommand(commands);		// Send "Set Profile" command

	if (getResult(responses, &length, TIMEOUT_LOCAL))
		return 1;	// NG -- maybe timeout

	DEBUG_PRINT(">setDefaultProfile()", responses);

	// parse response
	if (! strncmp(responses, "$PS=NG", 6))
		return 1;	// NG --  Can't set prfoile number

	return a3gsSUCCESS;	// OK
}

//***************************
//	getDefaultProfile
//
//	@description
//		Get default profile(APN) number
//	@return value
//		0 .. OK
//		1 .. NG
//	@param
//		profileNum : [OUT] profile number(1..a3gsMAX_PROFILE_NUMBER)
//	@note
//		
//***************************
int A3GS::getDefaultProfile(int* profileNum)
{
	char responses[20];  // Format "$PR=OK 99\n"
	int	length = sizeof(responses);
//--
	sendCommand("$PR");	// Send "Read Profile" command

	if (getResult(responses, &length, TIMEOUT_LOCAL))
		return 1;	// NG -- maybe timeout

	DEBUG_PRINT(">getDefaultProfile()", responses);

	// parse response
	*profileNum = atoi(responses + 7);

	return a3gsSUCCESS;  // Succeeded
}

//***************************
//	connectTCP
//
//	@description
//		Connect to server with TCP/IP connection
//	@return value
//		0 .. OK
//		1 .. NG
//	@param
//		server : server name or IP address(ex. "www,google.com", "192.161.10.1")
//		port : port number(ex. 80)
//	@note
//		
//***************************
int A3GS::connectTCP(const char* server, int port)
{
	char	responses[30];		// FORMAT: "$TC=OK\n" or "$TC=NG errno [info]\n"
	int	length = sizeof(responses);
//--
	// Check parameter size
	if (strlen(server) > a3gsMAX_HOST_LENGTH)
		return 1;	// NG -- too long host name
	// Make command and send it
	sprintf(gWorkBuffer, "$TC %s %d", server, port);
	sendCommand(gWorkBuffer);

	if (getResult(responses, &length, TIMEOUT_NETWORK))
		return 1;	// NG -- maybe timeout

	DEBUG_PRINT(">connectTCP()", responses);

	// parse response
	if (! strncmp(responses, "$TC=NG", 6))
		return 1;	// NG --  Can't connect

	// Change status
	_status = TCPCONNECTEDCLIENT;

	return a3gsSUCCESS;	// OK
}

//***************************
//	disconnectTCP
//
//	@description
//		Disconnect from server with current TCP/IP connection
//	@return value
//		0 .. OK
//		1 .. NG
//	@param
//		none
//	@note
//		
//***************************
int A3GS::disconnectTCP()
{
	char	responses[30];		// FORMAT: "$TD=OK\n" or "$TD=NG errno [info]\n"
	int	length = sizeof(responses);
//--
	// Send command
	sendCommand("$TD");

	if (getResult(responses, &length, TIMEOUT_NETWORK))
		return 1;	// NG -- maybe timeout

	DEBUG_PRINT(">disconnectTCP()", responses);

	// parse response
	if (! strncmp(responses, "$TD=NG", 6))
		return 1;	// NG --  Can't connect

	// Change status
	_status = READY;

	return a3gsSUCCESS;	// OK
}

//***************************
//	write
//
//	@description
//		Write byte into current TCP/IP connection
//	@return value
//		-1 .. NG (Error)
//		 1 .. OK
//	@param
//		c : byte data to write
//	@note
//		
//***************************
int A3GS::write(uint8_t c)
{
	char	responses[30];		// FORMAT: "$TW=OK nbytes\n" or "$TW=NG errno [info]\n"
	int	length = sizeof(responses);
//--
	// Make command and send it
	sendData("$TW \"");
	if (c < 0x20) {
		char	escaped[6];
		sprintf(escaped, "$x%02x", c);
		iemSerial.print(escaped); 
	}
	else {
		iemSerial.print((char)c); 
	}
	sendCommand("\"");

	if (getResult(responses, &length, TIMEOUT_NETWORK))
		return -1;	// NG -- maybe timeout

	DEBUG_PRINT(">write()", responses);

	// parse response
	if (! strncmp(responses, "$TW=NG", 6))
		return -1;	// NG --  Can't connect

	int	nbytes = atoi(responses + 7);

	return nbytes;	// OK
}

//***************************
//	write
//
//	@description
//		Write string into current TCP/IP connection
//	@return value
//		-1 .. NG (Error)
//		0 < .. OK (= wrote bytes)
//	@param
//		str : string(= '\0' terminated byte array) to write
//	@note
//		"str" must be encoded by $ escape sequence
//***************************
int A3GS::write(const char* str)
{
	char	responses[30];		// FORMAT: "$TW=OK nbytes" or "$TW=NG errno [info]\n"
	int	length = sizeof(responses);
//--
	if (strlen(str) > a3gsMAX_DATA_LENGTH)
		return -1;	// NG -- too large data

	// Make command and send it
	sendData("$TW \"");
	sendData(str);
	sendCommand("\"");

	if (getResult(responses, &length, TIMEOUT_NETWORK))
		return -1;	// NG -- maybe timeou\t

	DEBUG_PRINT(">write()", responses);

	// parse response
	if (! strncmp(responses, "$TW=NG", 6))
		return -1;	// NG --  Can't connect

	int	nbytes = atoi(responses + 7);

	return nbytes;	// OK
}

//***************************
//	write
//
//	@description
//		Write byte data into current TCP/IP connection
//	@return value
//		-1 .. NG (Error)
//		0 < .. OK (= wrote bytes)
//	@param
//		buffer : data to write(byte array)
//		sz : data size(in byte)
//	@note
//		
//***************************
int A3GS::write(const uint8_t* buffer, size_t sz)
{
	char	responses[30];		// FORMAT: "$TW=OK nbytes\n" or "$TW=NG errno [info]\n"
	int	length = sizeof(responses);
//--
	if (sz > (size_t)a3gsMAX_DATA_LENGTH)
		return -1;	// NG -- too large data

	// Make command and send it
	sendData("$TW \"");
	while (sz-- > 0) {
		if (*buffer < 0x20) {
			char	escaped[6];
			sprintf(escaped, "$x%02x", *buffer);
			iemSerial.print(escaped); 
		}
		else {
			iemSerial.print((char)*buffer); 
		}
		buffer++;
	}
	sendCommand("\"");

	if (getResult(responses, &length, TIMEOUT_NETWORK))
		return -1;	// NG -- maybe timeout

	DEBUG_PRINT(">write()", responses);

	// parse response
	if (! strncmp(responses, "$TW=NG", 6))
		return -1;	// NG --  Can't connect

	int	nbytes = atoi(responses + 7);

	return nbytes;	// OK
}

//***************************
//	read
//
//	@description
//		Read data from current TCP/IP connection
//	@return value
//		-1 .. NG (Error)
//		 0 .. NG (Closed connection by other side)
//		0 < .. OK (= read bytes)
//	@param
//		result : [OUT] read data(byte array, not '\0' terminated)
//		resultlength : data length(in byte)
//	@note
//		
//***************************
int A3GS::read(char* result, int resultlength)
{
	int	length = sizeof(gWorkBuffer);
	int	c;		//@R2.0 Change
//--
	if (resultlength > a3gsMAX_DATA_LENGTH)
		return -1;	// NG -- Bad parameter

	// Make command and send it
	sprintf(gWorkBuffer, "$TR %d", resultlength);
	sendCommand(gWorkBuffer);

	if (getResult(gWorkBuffer, &length, TIMEOUT_NETWORK))
		return -1;	// NG -- maybe timeout

	DEBUG_PRINT(">read()", gWorkBuffer);

	// Parse response
	if (! strncmp(gWorkBuffer, "$TR=NG", 6))
		return -1;	// NG --  Can't connect

	int nbytes = atoi(gWorkBuffer + 7);
	DEBUG_PRINT(">read() nbytes", nbytes);

	// Copy response body into "result" and set "resultlength"
	for (int n = 0; n < nbytes; n++) {
		while ((c = iemSerial.read()) < 0)
			;	// read until valid response
		if (n < resultlength - 1)
			result[n] = c;
	}
	discardUntil('\n');		// discard last '\n' -- @R2.0 Change
	if (nbytes > resultlength - 1)			//@R2.0 Change
		result[resultlength - 1] = '\0';
	else
		result[nbytes] = '\0';

	return nbytes;		// OK
}

//***************************
//	read
//
//	@description
//		Read byte from current TCP/IP connection
//	@return value
//		 -1 .. NG (Error)
//		 0..0xFF .. OK (= read byte)
//	@param
//		none
//	@note
//		This function is synchronized operation.
//		Change return value type at @R2.0
//***************************
int A3GS::read(void)
{
	int	length = sizeof(gWorkBuffer);
	int	c;		//@R2.0 Change
//--
	// Send command
	sendCommand("$TR 1");

	if (getResult(gWorkBuffer, &length, TIMEOUT_NETWORK))
		return (-1);	// NG -- maybe timeout

	DEBUG_PRINT(">read()", gWorkBuffer);

	// Parse response
	if (! strncmp(gWorkBuffer, "$TR=NG", 6))
		return (-1);	// NG --  Can't connect

	while ((c = iemSerial.read()) < 0)
		;	// read valid response
	discardUntil('\n');	// discard last '\n' -- @R2.0 Change

	return c;	// OK

}

//***
//  Private methods
//***

//  sendCommand() -- send command to iem with \n
void A3GS::sendCommand(const char* cmd)
{
	// Force to listen iemSerial at multiple SoftwareSerial using
	iemSerial.listen();

	// Send command to IEM with '\n'
	iemSerial.println(cmd); 
	DEBUG_PRINT("<sendCommand()", cmd);
}

//  sendData() -- send data to iem without \n
void A3GS::sendData(const char* data)
{
	// Force to listen iemSerial at multiple SoftwareSerial using
	iemSerial.listen();

	// Send data to IEM without '\n'
	iemSerial.print(data); 
	DEBUG_PRINT("<sendData()", data);
}

// discardUntl() -- discard characters from iemSerial until match specified character
void A3GS::discardUntil(const char match)
{
	int	c;		//@R2.0 Change
	uint32_t ts = millis();
	boolean	done = false;
//--
	while (! done) {
		if (millis() - ts >= TIMEOUT_LOCAL) {
			DEBUG_PRINT(">discardUntil()", "TIMEOUT");
			break;		// Timeout !
		}
		while ((c = iemSerial.read()) < 0)
			;	// read until valid response
		if (c == match)
			done = true;
	}
}
	
// getResult() -- get result from IEM
int A3GS::getResult(char *buf, int *len, uint32_t timeout)
{
	uint32_t ts = millis();
	boolean completed = false;
	int length = 0;
	while (! completed) {
		if (millis() - ts >= timeout) {
			DEBUG_PRINT(">getResult()", "TIMEOUT");
			return 1;		// NG -- Timeout
		}
		if (iemSerial.available() <= 0)
			continue;
		int c = iemSerial.read();		//@R2.0 Change
		if (c == 0x0a) {	// end of line
			if (buf[0] == '$')
				completed = true;	// got result
			else
				length = 0;			// got status, so retry
		}
		else if (length < *len - 1)
			buf[length++] = c;
	}
	buf[length] = '\0';
	*len = length;

	DEBUG_PRINT("<getResult", buf);

	return 0;	// OK
}

//	handleINT0() -- Interrupt Service Routine(ISR) of INT0
void A3GS::handleINT0(void)
{
	// disable INT0
	detachInterrupt(INTNO_SMS);

	(*SMShandler)();	// Call user handler(You can use other interrupts(UART, millis()..))

	// Clear hander
	SMShandler = NULL;
}

// END OF a3gs.cpp

