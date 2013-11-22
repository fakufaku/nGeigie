/*
  nGeigie.ino

Connection:
 * An Arduino Ethernet Shield
 * D2: The output pin of the Geiger counter (active low)
 * D3: The output pin of the Geiger counter (active low)
 based on NetRad 1.1.2

 created 18 Nov 2013
 updated 22 Nov 2013
 by Allan Lind: alind@joho.com
 Code from Lionel and Kalin of Safecast
 This code is in the public domain.
 */
 
#define USE_DISPLAY 1     // comment this line out if not using the display

#include <SPI.h>         // needed for Arduino versions later than 0018
#include <Ethernet.h>
#include <util.h>
#include <avr/eeprom.h>
#include <limits.h>
#include <avr/wdt.h>
#include <Wire.h>
#include "board_specific_settings.h"

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define OLED_CS 4
#define OLED_MOSI 5
#define OLED_CLK 6
#define OLED_RESET 7
#define OLED_DC 8
#define NUMFLAKES 10
#define XPOS 0
#define YPOS 1
#define DELTAY 2
#define LOGO16_GLCD_HEIGHT 16 
#define LOGO16_GLCD_WIDTH  16 
#define SEPARATOR	"--------------------"
#define DEBUG		0



static char VERSION[] = "1.2.0";

#define _SS_MAX_RX_BUFF 128 // RX buffer size

#ifdef  USE_DISPLAY
        Adafruit_SSD1306 display(OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);
#endif

// this holds the info for the device
// static device_t dev;
#define LINE_SZ 100

static char json_buf[LINE_SZ];

int freeRAM ()
{
	extern int __heap_start, *__brkval;
	int v;
	return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}


// Sampling interval (e.g. 60,000ms = 1min)
unsigned long updateIntervalInMillis = 0;

// The next time to feed
unsigned long nextExecuteMillis = 0;

// Event flag signals when a geiger event has occurred
volatile unsigned char eventFlag = 0;		// FIXME: Can we get rid of eventFlag and use counts>0?
int counts_per_sample;

// The last connection time to disconnect from the server
// after uploaded feeds
long lastConnectionTime = 0;

// The conversion coefficient from cpm to µSv/h
float conversionCoefficient = 0;

void onPulse()
{
	counts_per_sample++;
	eventFlag = 1;
}

unsigned long elapsedTime(unsigned long startTime);
unsigned long int  nSv ;


// holds the control info for the device
static devctrl_t ctrl;

static FILE uartout = {0};		// needed for printf





void setup() {       
                                    

	pinMode(pinSpkr, OUTPUT);
	pinMode(pinLED, OUTPUT);

	// fill in the UART file descriptor with pointer to writer.
//	fdev_setup_stream (&uartout, uart_putchar, NULL, _FDEV_SETUP_WRITE);

	// The uart is the standard output device STDOUT.
	stdout = &uartout ;

	// init command line parser
        Serial.begin(57600);


	// reset the Wiznet chip
	pinMode(resetPin, OUTPUT);
	digitalWrite(resetPin, HIGH);
	delay(20);
	digitalWrite(resetPin, LOW);
	delay(20);
	digitalWrite(resetPin, HIGH);


	// init the control info
	memset(&ctrl, 0, sizeof(devctrl_t));

	// enable watchdog to allow reset if anything goes wrong			
	wdt_enable(WDTO_8S);                                                
	// comment out to disable interrupts max 8S = 8 seconds


	#ifdef  USE_DISPLAY
			display.begin(SSD1306_SWITCHCAPVCC);
			display.display(); // show splashscreen
			delay(2000);
			display.clearDisplay();   // clears the screen and buffer
			  // draw a single pixel
			display.drawPixel(10, 10, WHITE);
			display.display();
			delay(2000);
	#endif
			Serial.println();
	// Set the conversion coefficient from cpm to µSv/h

// LND_7318:
// Reference:

			Serial.println("Sensor Model: LND 7318");
			Serial.println("Conversion factor: 344 cpm = 1 uSv/Hr");
			conversionCoefficient = 0.0029;
	                Serial.print(freeRAM());
	                Serial.println(" b free");

	#ifdef  USE_DISPLAY
							display.setTextSize(1);
							display.setTextColor(WHITE);
							display.setCursor(0,0);
							display.clearDisplay();
							display.println("Sensor model: LND7318");
							display.display();
	#endif


/**************************************************************************/
// Print out the current device ID
/**************************************************************************/
	printf_P(PSTR("Firmware_ver:\t%s\n"), VERSION);

 
	// Initiate a DHCP session


	Serial.println("Getting an IP address...");

        if (Ethernet.begin(macAddress) == 0)

	{
       		Serial.println("Failed to configure Ethernet using DHCP");
  // DHCP failed, so use a fixed IP address:
 
        	Ethernet.begin(macAddress, localIP);
	}

	Serial.print("local_IP:\t");
	Serial.println(Ethernet.localIP());

	// Attach an interrupt to the digital pin and start counting
	//
	// Note:
	// Most Arduino boards have two external interrupts:
	// numbers 0 (on digital pin 2) and 1 (on digital pin 3)
//	attachInterrupt(0, onPulse, interruptMode);                                // comment out to disable the GM Tube
	attachInterrupt(1, onPulse, interruptMode);                                // comment out to disable the GM Tube
	updateIntervalInMillis = updateIntervalInMinutes * 60000;                  // update time in ms

	unsigned long now = millis();
	nextExecuteMillis = now + updateIntervalInMillis;

	// Walk the dog
	wdt_reset();
	Serial.print("Device ID\t") ;
	Serial.println(ID) ;
	Serial.println("setup done.");
	Serial.println(SEPARATOR);

#ifdef  USE_DISPLAY
  display.setCursor(0,10);
 
  display.println("Local IP Address:");
  display.setCursor(0,20);
  display.println(Ethernet.localIP());
  display.display();
  delay(2000);
  
#endif
}


//**************************************************************************/
/*!
//  On each falling edge of the Geiger counter's output,
//  increment the counter and signal an event. The event
//  can be used to do things like pulse a buzzer or flash an LED
*/
/**************************************************************************/



void SendDataToServer(float CPM) {

	// Convert from cpm to ÂµSv/h with the pre-defined coefficient
	float uSv = CPM * conversionCoefficient;                   // convert CPM to Micro Sieverts Per Hour
        float f_nSv = CPM * conversionCoefficient * 1000;          // convert CPM to Nano Sieverts Per Hour


        nSv = (long) (f_nSv+0.5) ;                                 // Convert Floating Point to 32 Bit Integer


        char csvData[16];
        dtostrf(CPM, 0, 0, csvData);

        dtostrf(uSv, 0, 0, csvData);

#ifdef  USE_DISPLAY
        display.setTextSize(2);
        display.setTextColor(WHITE);
        display.setCursor(0,0);
        display.clearDisplay();
        display.println(uSv);
        display.setCursor(20,18);
        display.println("uSv/Hr");
        display.display();
#endif
  
	if (client.connected())
	{
		Serial.println("Disconnecting...");
		client.stop();
	}

	// Try to connect to the server
	Serial.println("Connecting to safecast.org ...");
	if (client.connect(serverIP, 80))
	{
		Serial.println("Connected");
		lastConnectionTime = millis();

		// clear the connection fail count if we have at least one successful connection
		ctrl.conn_fail_cnt = 0;
	}
	else
	{
		ctrl.conn_fail_cnt++;
		if (ctrl.conn_fail_cnt >= MAX_FAILED_CONNS)
		{
				ctrl.state = RESET;
		}
		printf("Failed. Retries left: %d.\r\n", MAX_FAILED_CONNS - ctrl.conn_fail_cnt);
		lastConnectionTime = millis();
		return;
	}

	// Convert from cpm to µSv/h with the pre-defined coefficient
	float DRE = CPM * conversionCoefficient;
        char CPM_string[16];
        dtostrf(CPM, 0, 0, CPM_string);


  
    // prepare the log entry
	memset(json_buf, 0, LINE_SZ);
	sprintf_P(json_buf, PSTR("{\"longitude\":\"%s\",\"latitude\":\"%s\",\"device_id\":\"%s\",\"value\":\"%s\",\"unit\":\"cpm\"}"),  \
	              lon, \
	              lat, \
	              ID,  \
	              CPM_string);
//	              CPM_string.c_str());


	int len = strlen(json_buf);
	json_buf[len] = '\0';
	Serial.println(json_buf);

	client.print("POST /safecast/index.php?api_key=");
	client.print(apiKey);
	client.println(" HTTP/1.1");
	client.println("User-Agent: Arduino");
	client.println("Host: 176.56.236.75");
	client.print("Content-Length: ");
	client.println(strlen(json_buf));
	client.println("Content-Type: application/json");
	client.println();
	client.println(json_buf);
	Serial.println("Disconnecting...");
	client.stop();
	Serial.println(SEPARATOR);
}
  
void loop() {                                             
	// Main Loop
	// Walk the dog only if we're not in RESET state. if we're in RESET,
	// then that means something happened to mess up our connection and
	// we will just let the device gracefully reset
	if (ctrl.state != RESET)
	{
		wdt_reset();
	}


	// maintain the DHCP lease, if needed
	Ethernet.maintain();

	if (DEBUG)
	{
		// Echo received strings to a host PC
		if (client.available())
		{
			char c = client.read();
			Serial.print(c);
		}

		if (client.connected() && (elapsedTime(lastConnectionTime) > 10000))
		{
			Serial.println();
			Serial.println("Disconnecting...");
			client.stop();
		}
	}

	// Add any geiger event handling code here
	if (eventFlag)
	{
		eventFlag = 0;				// clear the event flag for later use
//              Serial.print(".");                     // prints a dot accross the console for each click
//		tone(pinSpkr, 2000);			// beep the piezo speaker

		digitalWrite(pinLED, HIGH);		// flash the LED
		delay(7);
		digitalWrite(pinLED, LOW);

//		noTone(pinSpkr);			// turn off the speaker

	}

	// check if its time to update server.
	// elapsedTime function will take into account counter rollover.
	if (elapsedTime(lastConnectionTime) < updateIntervalInMillis)
	{
		return;
	}

	float CPM = (float)counts_per_sample / (float)updateIntervalInMinutes;
	counts_per_sample = 0;

        SendDataToServer(CPM);

}


/**************************************************************************/
// calculate elapsed time, taking into account rollovers
/**************************************************************************/
unsigned long elapsedTime(unsigned long startTime)
{
	unsigned long stopTime = millis();

	if (startTime >= stopTime)
	{
		return startTime - stopTime;
	}
	else
	{
		return (ULONG_MAX - (startTime - stopTime));
	}
}


void GetFirmwareVersion()
{
	printf_P(PSTR("Firmware_ver:\t%s\n"), VERSION);
}
