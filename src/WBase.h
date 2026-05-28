#ifndef WBase_h
#define WBase_h

#include "Main.h"
#include "base/MQTTMan.h"
#include "base/EventSourceMan.h"
#include "base/Application.h"

const char appDataPredefPassword[] PROGMEM = "ewcXoCt4HHjZUvY1";

// TODO : Rename WBase
class WBase : public Application
{
private:
  // TODO : Declare configuration properies

  // TODO : Declare run/status properties
  EventSourceMan _eventSourceMan;

  // TODO : Declare required private methods

  void setConfigDefaultValues();
  bool parseConfigJSON(JsonVariant json, bool fromWebPage = false);
  void fillConfigJSON(JsonVariant json, bool forSaveFile = false);
  void fillStatusJSON(JsonVariant json);
  bool appInit(bool reInit = false);
  HtmlPage getHTMLContent(WebPageForPlaceHolder wp);
  void appInitWebServer(WebServer &server);
  void appRun();

public:
  WBase();
};

#endif
