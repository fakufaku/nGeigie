#include <SoftwareSerial.h>
#include <a3gs.h>
#define LINE_SZ 90

const char *svr = "176.56.236.75";
const char *apiKey = "q1LKu7RQ8s5pmyxunnDW";
const char *lat = "35.6617" ;
const char *lon = "139.7447" ;
const char *ID = "47" ;
char res[150];

static char json_buf[LINE_SZ];
static char buf[LINE_SZ];

void setup()
{

  Serial.begin(9600);
  Serial.println("Ready.");
  Serial.print("Initializing.. ");

  	int CPM= 60;
   	char CPM_string[16];
        dtostrf(CPM, 0, 0, CPM_string);
    memset(json_buf, 0, LINE_SZ);
	sprintf_P(json_buf, PSTR("longitude:139.7447"),  \
	              lon, \
	              lat, \
	              ID,  \
	              CPM_string);
	              
  	int len = strlen(json_buf);
	json_buf[len] = '\0';

  if (a3gs.start() == 0 && a3gs.begin() == 0) {
    Serial.println("Succeeded.");
	if (a3gs.connectTCP(svr, 80) == 0) {
		a3gs.write("POST /safecast/index.php?api_key=q1LKu7RQ8s5pmyxunnDW"); 
		//a3gs.write(apiKey);
		//a3gs.write(" HTTP/1.0");
		a3gs.write("$n$n");
		//a3gs.write("$n$n");
                //a3gs.write("User-Agent: Arduino");
                //a3gs.write("$n$n");
                //a3gs.write("Host: 176.56.236.75");
                //a3gs.write("$n$n");
		//a3gs.write("Content-Length: ");
		//a3gs.write(strlen(json_buf));
    	           //Serial.println(strlen(json_buf));
                //a3gs.write("$n$n");
        	//a3gs.write("Content-Type: application/json");
        	//a3gs.write("$n$n");
        	a3gs.write(json_buf);
                a3gs.write("$n$n");
                //a3gs.write("$n$n");
	        // Serial.println(json_buf);
		a3gs.write("Disconnecting:$n");
		Serial.println("Sending POST finished");
		}
	}
}
void loop()
{}


