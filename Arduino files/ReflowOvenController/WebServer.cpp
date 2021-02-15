#include "mytypes.h"
#include "WebServer.h"
#include <stdint.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include "FS.h"
#include "config.h"

WiFiServer server(80);
void respondStatus(WiFiClient client);
void respondStatic(WiFiClient client, String req);
void  setMode(WiFiClient client, String postData);
void  setPid(WiFiClient client, String postData);

void webserver_init() {
  server.begin();
  SPIFFS.begin();

}
void webserver_handle() {
  // Check if a client has connected
  WiFiClient client = server.available();
  if (!client) {
    return;
  }

  // Wait until the client sends some data

  while (!client.available()) {
    delay(1);
  }

  // Read the first line of the request
  String req = client.readStringUntil('\r');
  if (req.indexOf("POST") != -1) {
    delay(10);
    client.setTimeout(500);
    req += client.readString();
  }

  client.flush();

  if (req.indexOf("/api/status.json") != -1) {
    respondStatus(client);
  } else if (req.indexOf("POST /api/setMode") == 0) {
    String postData = req.substring(req.indexOf("\r\n\r\n") + 4);
    setMode(client, postData);
  } else if (req.indexOf("POST /api/setPid") == 0) {
    String postData = req.substring(req.indexOf("\r\n\r\n") + 4);
    setPid(client, postData);
  } else {
    respondStatic(client, req);
  }
  delay(1);
  client.flush();
}

void respondStatus(WiFiClient client) {
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["tccj"] = tc_reading.cold_junction_temperature;
  root["tct"] = tc_reading.thermocouple_temperature;
  root["fault"] = tc_reading.fault;
  root["target"] = Setpoint;
  root["level"] = Output / TRIAC_PERIOD;
  switch (oven_status) {
    case  IDLE:
      root["state"] = "IDLE";
      break;
    case SOAK:
      root["state"] = "SOAK";
      break;
    case REFLOW:
      root["state"] = "REFLOW";
      break;
    case COOLING:
      root["state"] = "COOLING";
      break;
    case BAKING:
      root["state"] = "BAKING";
      break;

  }
  root["Kp"] = Kp;
  root["Ki"] = Ki;
  root["Kd"] = Kd;
  root["remainingSeconds"] = remaining_seconds;
  // Prepare the response
  String s = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nConnection: close\r\n\r\n";
  client.print(s);
  root.printTo(client);
  delay(1);
}

void respondStatic(WiFiClient client, String req) {
  byte readbuf[1000];
  if (req.indexOf("GET") != 0) {
    String r = "HTTP/1.1 405 METHOD NOT ALLOWED";
    client.print(r);
    return;
  }

  String s = "HTTP/1.1 200 OK\r\nContent-Type: ";
  if (req.indexOf(".css") != -1) {
    s += "text/css\r\n";
  } else if (req.indexOf(".htm") != -1) {
    s += "text/html\r\n";
  } else if (req.indexOf(".js") != -1) {
    s += "text/javascript\r\n";
  } else if (req.indexOf("GET / HTTP") != -1) {//root file
    s += "text/html\r\n";
  } else {
    String r = "HTTP/1.1 404 NOT FOUND\r\n\r\n404 NOT FOUND";
    client.print(r);
    return;
  }
  String fname = req.substring(4, req.indexOf(" HTTP"));
  if (fname.equals("/"))
    fname = "/index.html";

  File f = SPIFFS.open(fname, "r");
  if (!f) {
    String r = "HTTP/1.1 404 NOT FOUND\r\n\r\n404 NOT FOUND: '"+fname+"'";
    client.print(r);
    return;
  }
  s += "Content-length: ";
  s += String(f.size());
  s += "\r\nConnection: close\r\nCache-Control: max-age=31536000\r\n\r\n";
  client.print(s);

  while (f.available()) {
    int rsz=f.read(readbuf,sizeof(readbuf));
    client.write(readbuf,rsz);
    delay(1);
  }

  f.close();
}

void  setMode(WiFiClient client, String postData) {
  String s = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nConnection: close\r\n\r\n";

  if (postData.indexOf("action=STOP") != -1) {
    oven_status = IDLE;
    client.print(s);

  } else if (postData.indexOf("action=START") != -1) {
    oven_status = SOAK;
    client.print(s);
  }
  s = "HTTP/1.1 400 BAD REQUEST\r\nConnection: close\r\n\r\n";
  client.print(s);
}
void  setPid(WiFiClient client, String postData) {
  char buf[64];
  char *token;
  char* pEnd;


  postData.toCharArray(buf, sizeof(buf));
  buf[63] = NULL;
  String s = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nConnection: close\r\n\r\n";
  token = strtok(buf, "&");
  while (token != NULL) {
    if (*token == 'k') {
      if (*(token + 1) == 'p') {
        Kp = strtof(token + 3, &pEnd);
      } else if (*(token + 1) == 'i') {
        Ki = strtof(token + 3, &pEnd);
      } else if (*(token + 1) == 'd') {
        Kd = strtof(token + 3, &pEnd);
      }
    }
    token = strtok(NULL, "&");
  }
  tuningsChanged = true;
  client.print(s);
}

