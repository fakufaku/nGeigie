// A3GS sample sketch.15 -- connectTCP/disconnectTCP/read/write
//   A title is extracted from a homepage. 

#include <SoftwareSerial.h>
#include <a3gs.h>

const char *server = "api.safecast.org";  // URL to extract a title
const int port = 443;
char res[400];

boolean getTitle(char *res);

void setup()
{
  Serial.begin(9600);
  delay(3000);  // Wait for Start Serial Monitor
  Serial.println("Ready.");

 _redo:
  Serial.print("Initializing.. ");
  if (a3gs.start() == 0 && a3gs.begin() == 0) {
    Serial.println("Succeeded.");
    if (a3gs.connectTCP(server, port) != 0) {
      Serial.println("connectTCP() can't connect");
      goto _end;
    }
    // Send GET request
    a3gs.write("GET / HTTP/1.0$n");
    a3gs.write("HOST:");
    a3gs.write(server);
    a3gs.write("$n$n");
    // Receive response
    do {
      int nbytes;
      if ((nbytes = a3gs.read(res, sizeof(res)-1)) < 0) {
        Serial.println("read() failed");
        break;
      }
      res[nbytes] = '\0';
    } while (! getTitle(res));
  }
  else
    Serial.println("Failed.");

  Serial.println("Shutdown..");
  a3gs.end();
  a3gs.shutdown();

  delay(15000);
  goto _redo;  // Repeat

 _end:
  while (1) ;
}

void loop()
{
}

boolean getTitle(char *p)
{
  char  *title;

  while (*p != '\0') {
    if (*p++ != '<')
      continue;    // skip not tag
    if (strncmp((const char *)p, "title>", 6))
      continue;    // skip not title tag
    // title tag found
    p += 6;
    title = p;
    while (*p != '\0' && *p != '<')
      p++;
    *p = '\0';
    Serial.print(server);
    Serial.print(" : Page title is \"");
    Serial.print(title);
    Serial.println("\"");
    return true;
  }
  return false;
}

// END
