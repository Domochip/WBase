#include "Core.h"
#include <EEPROM.h>
#include "../Main.h" //for VERSION define
#include "Version.h" //for BASE_VERSION define

#include "data/index.html.gz.h"
#include "data/pure-min.css.gz.h"
#include "data/side-menu.css.gz.h"
#include "data/side-menu.js.gz.h"

void Core::setConfigDefaultValues() {};
bool Core::parseConfigJSON(JsonDocument &doc, bool fromWebPage = false) { return true; };
String Core::generateConfigJSON(bool clearPassword = false) { return String(); };
String Core::generateStatusJSON()
{
  JsonDocument doc;

  char sn[9];
#ifdef ESP8266
  sprintf_P(sn, PSTR("%08x"), ESP.getChipId());
#else
  sprintf_P(sn, PSTR("%08x"), (uint32_t)(ESP.getEfuseMac() << 40 >> 40));
#endif
  unsigned long minutes = millis() / 60000;

  doc["sn"] = sn;
  doc["baseversion"] = BASE_VERSION;
  doc["version"] = VERSION;
  doc["uptime"] = String((byte)(minutes / 1440)) + 'd' + (byte)(minutes / 60 % 24) + 'h' + (byte)(minutes % 60) + 'm';
  doc["freeheap"] = ESP.getFreeHeap();
#ifdef ESP8266
  doc["freestack"] = ESP.getFreeContStack();
  doc["flashsize"] = ESP.getFlashChipRealSize();
#else
  doc["freestack"] = uxTaskGetStackHighWaterMark(nullptr);
#endif

  String gs;
  serializeJson(doc, gs);

  return gs;
}
bool Core::appInit(bool reInit = false) { return true; };
const PROGMEM char *Core::getHTMLContent(WebPageForPlaceHolder wp)
{
  switch (wp)
  {
  case status:
    return status0htmlgz;
    break;
  case config:
    return config0htmlgz;
    break;
  };
  return nullptr;
}
size_t Core::getHTMLContentSize(WebPageForPlaceHolder wp)
{
  switch (wp)
  {
  case status:
    return sizeof(status0htmlgz);
    break;
  case config:
    return sizeof(config0htmlgz);
    break;
  };
  return 0;
}
void Core::appInitWebServer(WebServer &server, bool &shouldReboot, bool &pauseApplication)
{
  // root is index
  server.on("/", HTTP_GET, [&server]()
            {
    SERVER_KEEPALIVE_FALSE()
    server.sendHeader(F("Content-Encoding"), F("gzip"));
    server.send_P(200, PSTR("text/html"), indexhtmlgz, sizeof(indexhtmlgz)); });

  // Ressources URLs
  server.on("/pure-min.css", HTTP_GET, [&server]()
            {
    SERVER_KEEPALIVE_FALSE()
    server.sendHeader(F("Content-Encoding"), F("gzip"));
    server.sendHeader(F("Cache-Control"), F("max-age=604800, public"));
    server.send_P(200, PSTR("text/css"), puremincssgz, sizeof(puremincssgz)); });

  server.on("/side-menu.css", HTTP_GET, [&server]()
            {
    SERVER_KEEPALIVE_FALSE()
    server.sendHeader(F("Content-Encoding"), F("gzip"));
    server.sendHeader(F("Cache-Control"), F("max-age=604800, public"));
    server.send_P(200, PSTR("text/css"), sidemenucssgz, sizeof(sidemenucssgz)); });

  server.on("/side-menu.js", HTTP_GET, [&server]()
            {
    SERVER_KEEPALIVE_FALSE()
    server.sendHeader(F("Content-Encoding"), F("gzip"));
    server.sendHeader(F("Cache-Control"), F("max-age=604800, public"));
    server.send_P(200, PSTR("text/javascript"), sidemenujsgz, sizeof(sidemenujsgz)); });

  server.on("/fw.html", HTTP_GET, [this, &server]()
            {
    SERVER_KEEPALIVE_FALSE()
    server.sendHeader(F("Content-Encoding"), F("gzip"));
    server.send_P(200, PSTR("text/html"), fwhtmlgz, sizeof(fwhtmlgz)); });

  // Firmware POST URL allows to push new firmware ----------------------------
  server.on(
      "/fw", HTTP_POST, [&shouldReboot, &pauseApplication, &server]()
      {
    shouldReboot = !Update.hasError();
    if (shouldReboot) {
      SERVER_KEEPALIVE_FALSE()
      server.send(200, F("text/html"), F("Firmware Successfully Uploaded<script>setTimeout(function(){if('referrer' in document)window.location=document.referrer;},10000);</script>"));
    }
    else {
      //Upload failed so restart to Run Application in loop
      pauseApplication = false;
      //Prepare response
      String errorMsg(Update.getError());
      errorMsg+=' ';
#ifdef ESP8266
      switch(Update.getError()){
        case UPDATE_ERROR_WRITE:
          errorMsg=F("Flash Write Failed");
          break;
        case UPDATE_ERROR_ERASE:
          errorMsg=F("Flash Erase Failed");
          break;
        case UPDATE_ERROR_READ:
          errorMsg=F("Flash Read Failed");
          break;
        case UPDATE_ERROR_SPACE:
          errorMsg=F("Not Enough Space");
          break;
        case UPDATE_ERROR_SIZE:
          errorMsg=F("Bad Size Given");
          break;
        case UPDATE_ERROR_STREAM:
          errorMsg=F("Stream Read Timeout");
          break;
        case UPDATE_ERROR_MD5:
          errorMsg=F("MD5 Check Failed");
          break;
        case UPDATE_ERROR_FLASH_CONFIG:
          errorMsg=F("Flash config wrong");
          break;
        case UPDATE_ERROR_NEW_FLASH_CONFIG:
          errorMsg=F("New Flash config wrong");
          break;
        case UPDATE_ERROR_MAGIC_BYTE:
          errorMsg=F("Magic byte is wrong, not 0xE9");
          break;
        case UPDATE_ERROR_BOOTSTRAP:
          errorMsg=F("Invalid bootstrapping state, reset ESP8266 before updating");
          break;
        default:
          errorMsg=F("Unknown error");
          break;
      }
#else
      errorMsg=Update.errorString();
#endif
      SERVER_KEEPALIVE_FALSE()
      server.send(500, F("text/html"), errorMsg);
    } },
      [&pauseApplication, &server]()
      {
        HTTPUpload &upload = server.upload();
        if (upload.status == UPLOAD_FILE_START)
        {
          // stop to Run Application in loop
          pauseApplication = true;

#ifdef LOG_SERIAL
          LOG_SERIAL.printf("Update Start: %s\n", upload.filename.c_str());
#endif

#ifdef ESP8266
          if (!Update.begin((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000))
          {
#else
          if (!Update.begin())
          {
#endif
#ifdef LOG_SERIAL
            Update.printError(LOG_SERIAL);
#endif
          }
        }
        else if (upload.status == UPLOAD_FILE_WRITE)
        {
          if (Update.write(upload.buf, upload.currentSize) != upload.currentSize)
          {
#ifdef LOG_SERIAL
            Update.printError(LOG_SERIAL);
#endif
          }
        }
        else if (upload.status == UPLOAD_FILE_END)
        {
          if (Update.end(true))
          {
#ifdef LOG_SERIAL
            LOG_SERIAL.printf("Update Success: %uB\n", upload.totalSize);
          }
          else
          {
            Update.printError(LOG_SERIAL);
#endif
          }
        }
#ifdef ESP8266
        // Feed the dog otherwise big firmware won't pass
        ESP.wdtFeed();
#endif
        yield();
      });

  // reboot POST --------------------------------------------------------------
  server.on("/rbt", HTTP_POST, [&shouldReboot, &server]()
            {
    SERVER_KEEPALIVE_FALSE()
    server.send_P(200,PSTR("text/html"),PSTR("Reboot command received<script>setTimeout(function(){if('referrer' in document)window.location=document.referrer;},30000);</script>"));
    shouldReboot = true; });

  // reboot RescueMode POST ---------------------------------------------------
  server.on("/rbtrsc", HTTP_POST, [&shouldReboot, &server]()
            {
    SERVER_KEEPALIVE_FALSE()
    server.send_P(200,PSTR("text/html"),PSTR("Reboot in rescue command received<script>setTimeout(function(){if('referrer' in document)window.location=document.referrer;},30000);</script>"));
    //Set EEPROM for Rescue mode flag
    EEPROM.begin(4);
    EEPROM.write(0, 1);
    EEPROM.end();
    shouldReboot = true; });

  // 404 on not found ---------------------------------------------------------
  server.onNotFound([&server]()
                    {
    SERVER_KEEPALIVE_FALSE()
    server.send(404); });
}