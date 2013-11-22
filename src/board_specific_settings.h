// NOTE:
// Before uploading to your Arduino Ethernet board,
// please replace with your own settings

#define MAX_FAILED_CONNS 3


// Your API key (a public secure key is recommended) IWAKI HAWK 002
const char *apiKey = "buXEwRaCpBMer8jTZ9mp";
const char *lat = "37.0105" ;
const char *lon = "140.9253" ;
const char *ID = "41" ;

// REPLACE WITH A PROPER MAC ADDRESS
byte macAddress[] = { 0xDA, 0xD1, 0xBE, 0x60, 0x0D, 0x00 };


// Enter an IP address for your controller below.
// The IP address will be dependent on your local network:
EthernetClient client;
IPAddress localIP (192, 168, 100, 40);		// fallback local IP address if no DHCP
IPAddress serverIP(176, 56, 236, 75 );


// This is specific to the Uno Ethernet board
int pinSpkr = 1;	// pin number of piezo speaker
int pinLED = 0;		// pin number of event LED
int resetPin = A1;
//int radioSelect = A3;


// Update interval in minutes
const int updateIntervalInMinutes = 1;

typedef struct
{
  unsigned short addr;
  char lat[10];
  char lon[10];
  char ID[3];
  char firmware_version[8];
} device_t;

typedef struct
{
    unsigned char state;
    unsigned char conn_fail_cnt;
} devctrl_t;



enum states
{
    NORMAL = 0,
    RESET = 1
};

// Tube model
//const TubeModel tubeModel = LND_7313;

// Interrupt mode:
// * For most geiger counter modules: FALLING
// * Geiger Counter Twig by Seeed Studio: RISING
const int interruptMode = RISING;

// vim: set tabstop=4 shiftwidth=4 syntax=c foldmethod=marker :

