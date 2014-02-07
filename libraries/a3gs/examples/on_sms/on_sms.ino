// A3GS sample sketch.14 -- onSMSReceived

#include <SoftwareSerial.h>
#include "a3gs.h"

const int ledPin = 13;
volatile boolean received = false;

char msg[a3gsMAX_SMS_LENGTH+1], msn[a3gsMAX_MSN_LENGTH+1];

void ledOn(void)
{
  digitalWrite(ledPin, HIGH);  // LED on
  received = true;
}

void setup()
{
  Serial.begin(9600);
  delay(3000);  // Wait for Start Serial Monitor
  Serial.println("Ready.");

  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);  // LED off
  
  Serial.print("Initializing.. ");

  if (a3gs.start() == 0 && a3gs.begin() == 0) {
      Serial.println("Succeeded.");
      a3gs.onSMSReceived(ledOn);
  }
  else
    Serial.println("Failed.");
}

void loop()
{
  if (received) {
    Serial.println("SMS was received.");
    if (a3gs.readSMS(msg, sizeof(msg), msn, sizeof(msn)) == 0) {
      Serial.print("MSN: ");
      Serial.println(msn);
      Serial.print("SMS: ");
      Serial.println(msg);
    }
    digitalWrite(ledPin, LOW);  // LED off
    received = false;
    a3gs.onSMSReceived(ledOn);  // Re-set handler
  }
}
