#include "WBase.h"

//------------------------------------------
// Used to initialize configuration properties to default values
void WBase::setConfigDefaultValues()
{
  // TODO
  // property1 = 0;
  // property2 = F("test");
}

//------------------------------------------
// Parse JSON object into configuration properties
bool WBase::parseConfigJSON(JsonVariant json, bool fromWebPage /* = false */)
{
  // Be carefull about the type of the properties
  // when Json is coming from web page (fromWebPage = true), all properties are string
  // when Json is coming from savefile, property types are defined by the generateConfigJSON

  // TODO
  // JsonVariant jv;

  // example for numeric value
  // if ((jv = json[F("prop1")]).is<JsonVariant>()) // check if property is present
  //   property1 = jv; // if the property is a string, it will be automatically converted to a number (be carefull default value if conversion fail is 0)

  // example for string value
  // if ((jv = json[F("prop2")]).is<const char *>()) // do a more precise check about the type otherwise strlcpy will crash
  //   strlcpy(property2, jv, sizeof(property2));

  // example for boolean value
  // property3 = jv; // html checkbox will send "on" string or no json property so direct conversion fit the need

  return true;
}

//------------------------------------------
// Generate JSON from configuration properties
void WBase::fillConfigJSON(JsonVariant json, bool forSaveFile /* = false */)
{
  // TODO
  // json["p1"] = property1;
  // json["p2"] = property2;
}

//------------------------------------------
// Generate JSON of application status
void WBase::fillStatusJSON(JsonVariant json)
{
  // TODO
  // json["p1"] = property1;
  // json["p2"] = property2;
}

//------------------------------------------
// code to execute during initialization and reinitialization of the app
bool WBase::appInit(bool reInit /* = false */)
{
  // TODO
  // if (toto.enabled) _sendTimer.setInterval(SEND_PERIOD, [this]() {this->SendTimerTick();});

  return true;
}

//------------------------------------------
// code to register web request answer to the web server
void WBase::appInitWebServer(WebServer &server)
{
  // TODO
  // server.on(F("/getColor"), HTTP_GET, [this, &server]() {server.send(200, F("text/html"), GetColor());});

  // register SSEServer
  _sse.init(getAppIdChar(), server);
}

//------------------------------------------
// Run for timer
void WBase::appRun()
{
  // last time status event source was sent
  static unsigned long lastEvtSrcSentMillis;

  unsigned long currentMillis = millis();

  if (currentMillis - lastEvtSrcSentMillis >= 10000)
  {                                          // Check if it's time to send a new event
    _sse.broadcast("{\"Hello\":\"World\"}"); // Send a message to all connected clients
    lastEvtSrcSentMillis = currentMillis;
  }

  _sse.loop();

  // TODO : implement run tasks (receive from serial, run timer, etc.)
}

//------------------------------------------
// Constructor
WBase::WBase() : Application(CustomApp)
{
  // TODO : Initialize special structure or libraries in constructor
  // Note : most of the time, init is done during AppInit based on configuration
}