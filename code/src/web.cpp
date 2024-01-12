/* Copyright (C) 2024  Doctor Volt
 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <https://www.gnu.org/licenses></https:>.
*/

//#define AP_MODE /* Provides AP with IP 192.168.4.1 */
#define LOCAL_WEB /* Web UI located on PC */

#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
//#include <ESPAsyncWebSrv.h>
#ifndef LOCAL_WEB
  #include <SPIFFS.h>
#endif
#include <mdns.h>

// mdns hostname
const char *hostname = "mousecam.local";

void handleWebMessage(const char *data, size_t len);
void on_connect() __attribute__((weak));
void on_disconnect(); // __attribute__((weak));

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len)
{
  switch (type)
  {
  case WS_EVT_CONNECT:
    // Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
    on_connect();
    break;
  case WS_EVT_DISCONNECT:
    // Serial.printf("WebSocket client #%u disconnected\n", client->id());
    on_disconnect();
    break;
  case WS_EVT_DATA:
    // handleWebSocketMessage(arg, data, len);
    {
      AwsFrameInfo *info = (AwsFrameInfo *)arg;
      if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)
      {
        handleWebMessage((const char *)data, len);
      }
    }
  }
}

String processor(const String &var)
{
  // Serial.print("processor");
  // Serial.println(var);
  if (var == "STATE")
  {
  }
  return "Hello";
}

void web_init(const char *ssid, const char *password)
{
  // Connect to Wi-Fi
#ifdef AP_MODE
  WiFi.softAP(ssid, password);;
  IPAddress IP = WiFi.softAPIP();
  Serial.printf("AP IP address: %s\n", IP.toString());
#else  
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.println("WiFi Connected");
#endif  

  // Assign DNS name
  mdns_init();
  mdns_hostname_set(hostname);

  ws.onEvent(onEvent);
  server.addHandler(&ws);

// Initialize SPIFFS
#ifndef LOCAL_WEB
  Serial.printf("Open http://%s in browser\r\n", hostname);
  if (!SPIFFS.begin(true))
  {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  server.serveStatic("/", SPIFFS, "/");
#endif

// Magic to allow access to event server from anywhere
  DefaultHeaders::Instance().addHeader(F("Access-Control-Allow-Origin"), F("*"));
// DefaultHeaders::Instance().addHeader(F("Access-Control-Allow-Credentials"), F("true"));
  // Start server
  server.begin();
}

void web_write(const char *data)
{
  while (!ws.availableForWriteAll())
  {
    Serial.println("Waiting for websocket");
    vTaskDelay(10);
  }
  ws.textAll(data);
}

void web_cleanup()
{
  ws.cleanupClients();
}
