#ifndef WifiMan_h
#define WifiMan_h

#include "../Main.h"
#include "Application.h"

#include <Ticker.h>
#ifdef ESP8266
#include <ESP8266mDNS.h>
#else
#include <ESPmDNS.h>
#endif
#include <DNSServer.h>

class WifiMan : public Application
{

private:
  // Configuration Properties
  char ssid[32 + 1] = {0};
  char password[64 + 1] = {0};
  char hostname[24 + 1] = {0};
  uint32_t ip = 0;
  uint32_t gw = 0;
  uint32_t mask = 0;
  uint32_t dns1 = 0;
  uint32_t dns2 = 0;

// Run properties
#ifdef ESP8266
  WiFiEventHandler _discoEventHandler;
#endif
  // Station connection state
  enum class StaState : uint8_t
  {
    Off,        // no station configured (AP only)
    Connecting, // WiFi.begin() called, waiting for connection (AP enabled)
    Connected,  // connected (AP disabled)
    WaitRetry   // connection attempt failed, waiting before next attempt (AP enabled)
  };

  DNSServer *_dnsServer = nullptr;
  uint8_t _apChannel = 2;
  volatile StaState _staState = StaState::Off;
  uint32_t _staStateMillis = 0; // millis() when _staState was last changed
  bool _needRefreshWifi = false;
  Ticker _refreshTicker;
  uint8_t _refreshPeriod = 60;     // try to reconnect as station mode every 60 seconds
  uint8_t _reconnectDuration = 20; // duration to try to connect to Wifi in seconds
  uint16_t _connectionCount = 0;   // count of successful WiFi connections
  uint8_t _lastDiscoReason = 0;    // last WiFi disconnection reason code

  void enableAP(bool force = false);
  void setStaState(StaState state);
  void refreshWiFi();

  void setConfigDefaultValues();
  bool parseConfigJSON(JsonVariant json, bool fromWebPage = false);
  void fillConfigJSON(JsonVariant json, bool forSaveFile = false);
  void fillStatusJSON(JsonVariant json);
  bool appInit(bool reInit = false);
  void appInitWebServer(WebServer &server);
  void appRun();

public:
  WifiMan() : Application(WifiManApp) {};
  // helper to convert IPAddress to const char*
  static const char *ipToCString(IPAddress ip);
  // helper to get MAC address as const char* (cached)
  static const char *getMacAddress();
  void mqttPublishHassDiscovery(HassDiscoveryCtx &ctx);
};

#endif
