#ifndef WBase_h
#define WBase_h

#include "Main.h"
#include "base/MQTTMan.h"
#include "base/SSEServer.h"
#include "base/Application.h"

// TODO : Rename WBase
class WBase : public Application
{
private:
  // TODO : Declare configuration properies

  // TODO : Declare run/status properties
  SSEServer _sse;

  // TODO : Declare required private methods

  void setConfigDefaultValues();
  bool parseConfigJSON(JsonVariant json, bool fromWebPage = false);
  void fillConfigJSON(JsonVariant json, bool forSaveFile = false);
  void fillStatusJSON(JsonVariant json);
  bool appInit(bool reInit = false);
  void appInitWebServer(WebServer &server);
  void appRun();

public:
  WBase();
};

#endif
