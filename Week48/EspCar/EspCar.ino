#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <FS.h>

// Copy the network credentials from your working ESP sketch before uploading.
const char* ssid="Titenet-IoT";
const char* password="7kDtaphg";
ESP8266WebServer server(80);

// Each accepted HTTP request becomes exactly one newline-terminated UART message.
void sendLine(String message){
  Serial.println(message);
  server.sendHeader("Cache-Control","no-store");
  server.send(200,"text/plain","Sent");
}

// Validate the complete query before sending a number to the Mega.
void sendNumber(const char* prefix,int low,int high){
  String text=server.arg("value");
  char* end;
  long n=strtol(text.c_str(),&end,10);
  if(!text.length() || *end || n<low || n>high){ // ERROR handling
    server.send(400,"text/plain","Invalid value"); return; // ERROR handling
  }
  sendLine(String(prefix)+n);
}

void setup(){
  // UART0 carries commands at the same baud rate as the Mega.
  Serial.begin(9600);
  if(!SPIFFS.begin()) return; // ERROR handling: files must be available before serving the page
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid,password);
  while(WiFi.status()!=WL_CONNECTED) delay(200);
  Serial.print("IP "); Serial.println(WiFi.localIP()); // Read the assigned address on the ESP monitor at startup.

  // Keep the original compass endpoint and use matching routes for all cardinal buttons.
  server.on("/compass",HTTP_GET,[]{sendNumber("degree:",-360,360);});
  server.on("/speed",HTTP_GET,[]{sendNumber("speed:",0,255);});
  server.on("/findNorth",HTTP_GET,[]{sendLine("find:North");});
  server.on("/findEast",HTTP_GET,[]{sendLine("find:East");});
  server.on("/findSouth",HTTP_GET,[]{sendLine("find:South");});
  server.on("/findWest",HTTP_GET,[]{sendLine("find:West");});
  // Signed distances encode both the requested direction and distance.
  server.on("/forwards10",HTTP_GET,[]{sendLine("drive:10");});
  server.on("/forwards50",HTTP_GET,[]{sendLine("drive:50");});
  server.on("/backwards10",HTTP_GET,[]{sendLine("drive:-10");});
  server.on("/backwards50",HTTP_GET,[]{sendLine("drive:-50");});
  server.on("/ping",HTTP_GET,[]{sendLine("ping");}); // ERROR handling: relay browser heartbeat
  // Serve the existing project file types out of the uploaded data directory.
  server.serveStatic("/script.js",SPIFFS,"/script.js");
  server.serveStatic("/style.css",SPIFFS,"/style.css");
  server.serveStatic("/",SPIFFS,"/index.html");
  server.begin();
}
void loop(){
  // Dispatch pending HTTP requests, including heartbeat pings during a drive.
  server.handleClient();
}
