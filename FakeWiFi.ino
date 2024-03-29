/*
 * Fake WiFi hotspot to *strongly* discourage wifi usage in private or public areas where desired.
 * Permission is granted to freely use and distribute so long as this notice is left intact.
 * 
 * Please consider where this is used.  Locations with minor children - you should modify the landing page to be 
 * appropriate.  Otherwise for adult locations like bars and clubs, feel free to use the default landing page :)
 * 
 * Written by: John Rogers   john at wizworks dot net     http://wizworks.net on June 23rd 2018
 * 
 */

#include <FS.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include "PString.h"
#include "Streaming.h"
/*
Wemos D1 Mini Built in LED is GPIO2  2
ESP-WROOM-02  Built in LED is GPIO16 16

*/
int LED = 16;
const IPAddress apIp(192, 168, 0, 1);
DNSServer dnsServer;
ESP8266WebServer server(80);

struct PrintMac
{
  const uint8_t* mac;
};

Print&
operator<<(Print& p, const PrintMac& pmac)
{
  const uint8_t* mac = pmac.mac;
  for (int i = 0; i < 6; ++i) {
    if (i > 0) {
      p << ':';
    }
    int b = mac[i];
    if (b < 0x10) {
      p << '0';
    }
    p << _HEX(b);
  }
  return p;
}

File logfile;
char logbuf[256];
PString logline(logbuf, sizeof(logbuf));
void
appendLog()
{
  Serial << millis() << ' ' << logline << '\n';
  logfile << millis() << ' ' << logline << '\n';
  logline.begin();
}

void
wifiStaConnect(const WiFiEventSoftAPModeStationConnected& evt)
{
  logline << PrintMac{evt.mac} << " assoc";
  appendLog();
  //digitalWrite(LED, HIGH);
}
WiFiEventHandler wifiStaConnectHandler;

void
wifiStaDisconnect(const WiFiEventSoftAPModeStationDisconnected& evt)
{
  logline << PrintMac{evt.mac} << " deassoc";
  appendLog();
  digitalWrite(LED, HIGH);
}
WiFiEventHandler wifiStaDisconnectHandler;

void
httpDefault()
{
  //logline << server.client().remoteIP() << " redirect";
  //appendLog();
  server.sendHeader("Location", "http://freewifi.lan/", true);
  server.send(302, "text/plain", "");
  server.client().stop();
}

void
httpHome()
{
  if (server.hostHeader() != String("freewifi.lan")) {
    return httpDefault();
  }

  //logline << server.client().remoteIP() << " home";
  //appendLog();
  File file = SPIFFS.open("/index.htm.gz", "r");
  server.streamFile(file, "text/html");
  file.close();
}

void
httpTower()
{
  File file = SPIFFS.open("/tower.png", "r");
  server.streamFile(file, "image/png");
  file.close();
}

void
httpConnect()
{
  logline << server.client().remoteIP() << " connect " << server.arg("email");
  appendLog();
  File file = SPIFFS.open("/connect.htm.gz", "r");
  server.streamFile(file, "text/html");
  file.close();
  digitalWrite(LED, HIGH);
}

void
httpBird()
{
  logline << server.client().remoteIP() << " bird has been flipped!";
  appendLog();
  File file = SPIFFS.open("/bird.jpg", "r");
  server.streamFile(file, "image/jpeg");
  file.close();
  digitalWrite(LED, HIGH);
  delay(3000);
}

void
httpPureCss()
{
  File file = SPIFFS.open("/pure.css.gz", "r");
  server.streamFile(file, "text/css");
  file.close();
}

void
httpLog()
{
  logline << server.client().remoteIP() << " log";
  appendLog();
  logfile.seek(0, SeekSet);
  server.streamFile(logfile, "text/plain");
  logfile.seek(0, SeekEnd);
}

void
setup()
{
  delay(10000);
  Serial.begin(115200);
  Serial.println();
  Serial.println();
  pinMode(LED, OUTPUT);
  SPIFFS.begin();
  logfile = SPIFFS.open("/log.txt", "a+");

  WiFi.persistent(false);
  WiFi.disconnect(true);
  wifiStaConnectHandler = WiFi.onSoftAPModeStationConnected(wifiStaConnect);
  wifiStaDisconnectHandler = WiFi.onSoftAPModeStationDisconnected(wifiStaDisconnect);
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIp, apIp, IPAddress(255, 255, 255, 0));
  WiFi.softAP("FREE WIFI", nullptr, 1);

  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(53, "*", apIp);

  server.on("/", httpHome);
  server.on("/tower.png", httpTower);
  server.on("/connect", httpConnect);
  server.on("/bird.jpg", httpBird);
  server.on("/pure.css", httpPureCss);
  server.on("/log.txt", httpLog);
  server.onNotFound(httpDefault);
  server.begin();

  Serial << "ready" << endl;
}

void
loop()
{
  digitalWrite(LED, LOW);
  dnsServer.processNextRequest();
  server.handleClient();
}
