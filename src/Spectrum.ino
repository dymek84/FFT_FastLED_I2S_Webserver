#pragma once

#include "patternsstripe.h"
#include <WiFi.h>
#include <WebSocketsServer.h>
#include <WebServer.h>

#include <WiFiMulti.h>
#include <WiFiClientSecure.h>
#include "html.h"
const char *ssid = "swiatkiewicz";
const char *password = "11111123";
WebSocketsServer webSocket = WebSocketsServer(81);
WebServer server; // 80 default

// default values. You will change them via the Web interface
uint8_t brightness = 25;
uint32_t duration = 10000; // (10s) duration of the effect in the loop
uint8_t effect = 0;

bool isPlay = false;
bool isLoopEffect = false;
bool isRandom = false;

// effects that will be in the loop
uint8_t favEffects[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33};
uint8_t numFavEffects = sizeof(favEffects);

uint32_t lastChange;
uint8_t currentEffect = 0;

void loop()
{
  EVERY_N_SECONDS(1)
  {
    tickslow++;
    Serial.println("tickslow: " + String(tickslow));
  }
  loopPatternStripe();
  FastLED.show();
}

void setup()
{
  Serial.begin(115200);
  Serial.println("Starting Spectrum Analyzer");
  Serial.println("Setup start on core: " + String(xPortGetCoreID()));
  setupPatternStripe();
  xTaskCreatePinnedToCore(
      http,   /* Function to implement the task */
      "loop", /* Name of the task */
      10000,  /* Stack size in words */
      NULL,   /* Task input parameter */
      0,      /* Priority of the task */
      NULL,   /* Task handle. */
      0);     /* Core where the task should run */
}

void http(void *pvParameters)
{
  Serial.println("http start on core: " + String(xPortGetCoreID()));
  WiFi.begin(ssid, password);
  Serial.println("");
  int counterWifi = 0;
  // wait for connection
  while (WiFi.status() != WL_CONNECTED)
  {
    counterWifi++;
    if (counterWifi > 20)
    {
      WiFi.disconnect();
      WiFi.begin(ssid, password);
      counterWifi = 0;
    }
    delay(500);
    Serial.print(".");
  }
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP()); // IP adress assigned to your ESP

  server.on("/", []()
            { server.send(200, "text/html", html); });

  server.begin();
  webSocket.begin();

  // binding callback function
  webSocket.onEvent(webSocketEvent);
  while (true)
  {

    httpLoop();
  }
}

void hexdump(const void *mem, uint32_t len, uint8_t cols = 16)
{
  const uint8_t *src = (const uint8_t *)mem;
  Serial.printf("\n[HEXDUMP] Address: 0x%08X len: 0x%X (%d)", (ptrdiff_t)src, len, len);
  for (uint32_t i = 0; i < len; i++)
  {
    if (i % cols == 0)
    {
      Serial.printf("\n[0x%08X] 0x%08X: ", (ptrdiff_t)src, i);
    }
    Serial.printf("%02X ", *src);
    src++;
  }
  Serial.printf("\n");
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length)
{

  switch (type)
  {
  case WStype_DISCONNECTED:
    Serial.printf("[%u] Disconnected!\n", num);
    break;
  case WStype_CONNECTED:
  {
    IPAddress ip = webSocket.remoteIP(num);
    Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);

    // send message to client
    webSocket.sendTXT(num, "Connected");
  }
  break;
  case WStype_TEXT:
    Serial.printf("Client[%u] Received: %s\n", num, payload);
    messageHandler(num, payload, length);
    break;
  case WStype_BIN:
    Serial.printf("[%u] get binary length: %u\n", num, length);
    hexdump(payload, length);

    // send message to client
    // webSocket.sendBIN(num, payload, length);
    break;
  case WStype_ERROR:
  case WStype_FRAGMENT_TEXT_START:
  case WStype_FRAGMENT_BIN_START:
  case WStype_FRAGMENT:
  case WStype_FRAGMENT_FIN:
    break;
  }
}
void httpLoop()
{
  webSocket.loop();      // constantly check for websocket events
  server.handleClient(); // run the web server
}

void messageHandler(uint8_t num, uint8_t *payload, size_t length)
{

  String getData;
  String sendData;

  for (int i = 0; i < length; i++)
  {
    if (!isdigit(payload[i]))
      continue;
    getData += (char)payload[i];
  }

  switch (payload[0])
  {
  // effect
  case 'E':
    isPlay = true;
    effect = getData.toInt();
    Serial.println(getData);

    currentEffect = getData.toInt() - 1; // so that the loop starts from the current one

    sendData = "E_" + getData;
    webSocket.broadcastTXT(sendData);

    // setEffect(effect);
    break;
  // brightness
  case 'B':
    Serial.println(getData);
    brightness = map(getData.toInt(), 0, 100, 0, 255);

    sendData = "B_" + getData;
    webSocket.broadcastTXT(sendData);

    LEDS.setBrightness(brightness);
    break;
  // duration
  case 'D':
    Serial.println(getData);
    duration = (getData.toInt() * 1000);

    sendData = "D_" + getData;
    webSocket.broadcastTXT(sendData);
    break;
  // play
  case 'P':
    if (getData == "1")
    {
      // isPlay = true;
      if (effect == 0)
      {
        effect = 1;
      }
      sendData = "E_" + String(effect, DEC);
      webSocket.broadcastTXT(sendData);
    }
    else
    {
      // isPlay = false;
    }
    sendData = "P_" + getData;
    webSocket.broadcastTXT(sendData);
    break;
  // loop
  case 'L':
    if (getData == "1")
    {
      // isPlay = true;
      isLoopEffect = true;
      isRandom = false;
    }
    else
    {
      isLoopEffect = false;
    }
    sendData = "L_" + getData;
    webSocket.broadcastTXT(sendData);
    break;
  // random
  case 'R':
    if (getData == "1")
    {
      // isPlay = true;
      isLoopEffect = false;
      isRandom = true;
    }
    else
    {
      isRandom = false;
    }
    sendData = "R_" + getData;
    webSocket.broadcastTXT(sendData);
    break;
  }
  Serial.println("Sent: " + sendData);
}