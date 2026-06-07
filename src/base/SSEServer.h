#ifndef SSEServer_h
#define SSEServer_h

#include "../Main.h"
#ifdef ESP8266
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
using WebServer = ESP8266WebServer;
#else
#include <WiFi.h>
#include <WebServer.h>
#endif

#include <Ticker.h>
#include <ArduinoJson.h>

class EventSourceMan
{
private:
#if EVTSRC_ENABLED
    WiFiClient _clients[EVTSRC_MAX_CLIENTS];

    void handleSubscription(WebServer &server);
    void forEach(std::function<void(WiFiClient &, uint8_t)> action);

#if EVTSRC_KEEPALIVE_ENABLED
    bool _needKeepAlive = false;
    Ticker _keepAliveTicker;

    void sendKeepAlive();
#endif
#endif

public:
#if EVTSRC_ENABLED
    void init(char appIdChar, WebServer &server);
    void broadcast(const char *message, const char *eventType = "message");
    void broadcast(JsonVariantConst message, const char *eventType = "message");

#if EVTSRC_KEEPALIVE_ENABLED

    void run();
#endif
#endif
};

#endif