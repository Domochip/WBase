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

class SSEServer
{
private:
#if SSE_SERVER_ENABLED
    WiFiClient _clients[SSE_SERVER_MAX_CLIENTS];

    void handleSubscription(WebServer &server);
    void forEach(std::function<void(WiFiClient &, uint8_t)> action);

#if SSE_SERVER_KEEPALIVE
    bool _needKeepAlive = false;
    Ticker _keepAliveTicker;

    void sendKeepAlive();
#endif
#endif

public:
#if SSE_SERVER_ENABLED
    void init(char appIdChar, WebServer &server);
    void broadcast(const char *message, const char *eventType = "message");
    void broadcast(JsonVariantConst message, const char *eventType = "message");

#if SSE_SERVER_KEEPALIVE

    void run();
#endif
#endif
};

#endif