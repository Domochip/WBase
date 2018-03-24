#include "WirelessBase.h"

//------------------------------------------
//Used to initialize configuration properties to default values
void MyApplication::SetConfigDefaultValues(){
    //TODO
    //property1 = 0;
    //property2 = F("test");
};
//------------------------------------------
//Parse JSON object into configuration properties
void MyApplication::ParseConfigJSON(JsonObject &root){
    //TODO
    //if (root["prop1"].success()) property1 = root[F("prop1")];
    //if (root["prop2"].success()) strlcpy(property2, root["prop2"], sizeof(property2));
};
//------------------------------------------
//Parse HTTP POST parameters in request into configuration properties
bool MyApplication::ParseConfigWebRequest(AsyncWebServerRequest *request)
{
    //TODO
    // if (!request->hasParam(F("prop1"), true))
    // {
    //     request->send(400, F("text/html"), F("prop1 missing"));
    //     return false;
    // }
    //if (request->hasParam(F("prop1"), true)) property1 = request->getParam(F("prop1"), true)->value().toInt();
    //if (request->hasParam(F("prop2"), true) && request->getParam(F("prop2"), true)->value().length() < sizeof(property2)) strcpy(property2, request->getParam(F("prop2"), true)->value().c_str());

    return true;
};
//------------------------------------------
//Generate JSON from configuration properties
String MyApplication::GenerateConfigJSON(bool forSaveFile = false)
{
    String gc('{');
    //TODO
    // gc = gc + F("\"p1\":") + (property1 ? true : false);
    // gc = gc + F("\"p2\":\"") + property2 + '"';

    gc += '}';

    return gc;
};
//------------------------------------------
//Generate JSON of application status
String MyApplication::GenerateStatusJSON()
{
    String gs('{');

    //TODO
    // gs = gs + F("\"p1\":") + (property1 ? true : false);
    // gs = gs + F("\"p2\":\"") + property2 + '"';

    gs += '}';

    return gs;
};
//------------------------------------------
//code to execute during initialization and reinitialization of the app
bool MyApplication::AppInit(bool reInit)
{
    //TODO
    //if (toto.enabled) _sendTimer.setInterval(SEND_PERIOD, [this]() {this->SendTimerTick();});

    return true;
};
//------------------------------------------
//code to register web request answer to the web server
void MyApplication::AppInitWebServer(AsyncWebServer &server, bool &shouldReboot, bool &pauseApplication){
    //TODO
    //server.on("/getColor", HTTP_GET, [this](AsyncWebServerRequest * request) {request->send(200, F("text/html"), GetColor());});
};

//------------------------------------------
//Run for timer
void MyApplication::AppRun()
{
    //TODO : implement run tasks (receive from serial, run timer, etc.)
}

//------------------------------------------
//Constructor
MyApplication::MyApplication(char appId, String appName) : Application(appId, appName)
{
    //TODO : Initialize special structure or libraries in constructor
    //Note : most of the time, init is done during AppInit based on configuration
}