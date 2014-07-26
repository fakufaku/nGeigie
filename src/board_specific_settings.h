// NOTE:
// Before uploading to your Arduino Ethernet board,
// please replace with your own settings

#define MAX_FAILED_CONNS 3


// Your API key (a public secure key is recommended) IWAKI HAWK 002
const char *apiKey = "<your_api_key>" ;
const char *lat = "<your_latitude>" ;
const char *lon = "<your_longitude>" ;
const char *ID = "<your_device_number>" ;

// REPLACE WITH A PROPER MAC ADDRESS
byte macAddress[] = { 0xDA, 0xD1, 0xBE, 0x60, 0x0D, 0x00 };


// Enter an IP address for your controller below.
// The IP address will be dependent on your local network:
EthernetClient client;
IPAddress localIP (10, 0, 0, 5);		// fallback local IP address if no DHCP
//IPAddress serverIP(176, 56, 236, 75 );  // Lionel's temporary gateway
IPAddress serverIP(107, 161, 164, 166);   // Official gateway


// This is specific to the Uno Ethernet board
int pinSpkr = 6;	// pin number of piezo speaker
int pinLED = 7;		// pin number of event LED
int resetPin = A1;
//int radioSelect = A3;


// Update interval in minutes
const int updateIntervalInMinutes = 5;

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

