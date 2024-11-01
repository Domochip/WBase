#ifndef WBase_h
#define WBase_h

#include "Main.h"
#include "base/MQTTMan.h"
#include "base/EventSourceMan.h"
#include "base/Application.h"

const char appDataPredefPassword[] PROGMEM = "ewcXoCt4HHjZUvY1";

#include "data/status2.html.gz.h"
#include "data/config2.html.gz.h"

// TODO : Rename WBase
class WBase : public Application
{
private:
  // TODO : Declare configuration properies

  // TODO : Declare run/status properties
  EventSourceMan _eventSourceMan;

  // TODO : Declare required private methods

  void setConfigDefaultValues();
  bool parseConfigJSON(JsonDocument &doc, bool fromWebPage);
  String generateConfigJSON(bool forSaveFile);
  String generateStatusJSON();
  bool appInit(bool reInit);
  const PROGMEM char *getHTMLContent(WebPageForPlaceHolder wp);
  size_t getHTMLContentSize(WebPageForPlaceHolder wp);
  void appInitWebServer(WebServer &server);
  void appRun();

public:
  WBase();
};

#endif
