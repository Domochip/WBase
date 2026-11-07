#include "Application.h"
#include "ReleaseInfoParser.h"

Application *Application::_applicationList[3] = {nullptr, nullptr, nullptr};

void Application::HassDiscoveryCtx::publishEntity(JsonDocument &json, const String &type, const __FlashStringHelper *uniqueIdSuffix, bool withStandardAvail)
{
  static const __FlashStringHelper *standardAvailabilityJSON = F("{\"topic\":\"~/connected\",\"value_template\":\"{{ iif((value | int(0)) > 0, 'online', 'offline') }}\"}");

  String uniqueId = uniqueIdPrefix + '_' + uniqueIdSuffix;

  // prepare topic: <hassDiscoveryPrefix>/<type>/<uniqueId>/config
  String topic;
  topic.reserve(strlen(hassDiscoveryPrefix) + type.length() + uniqueId.length() + 9); // 9 = "/" + "/" + "/config"
  topic += hassDiscoveryPrefix;
  topic += '/';
  topic += type;
  topic += '/';
  topic += uniqueId;
  topic += F("/config");

  // complete payload with common fields
  json["~"] = mqttMan.getBaseTopic();
  if (withStandardAvail)
    json[F("availability")] = serialized(availabilityJSON ? availabilityJSON : standardAvailabilityJSON);
  json[F("device")] = serialized(device);
  json[F("unique_id")] = uniqueId;

  // publish to MQTT
  mqttMan.publish(topic.c_str(), json, true);
}

Application::Application(AppId appId) : _appId(appId)
{
  _applicationList[_appId] = this;
}

char Application::getAppIdChar()
{
  return '0' + _appId;
}

const __FlashStringHelper *Application::getAppIdName()
{
  if (_appId == CoreApp)
    return F("Core");

  if (_appId == WifiManApp)
    return F("WiFi");

  return F(CUSTOM_APP_MODEL);
}

const __FlashStringHelper *Application::getMqttTopicSuffix()
{
  if (_appId == CoreApp)
    return F("/Core");

  if (_appId == WifiManApp)
    return F("/WiFi");

  return F("/App");
}

bool Application::saveConfig()
{
  char configPath[32];
  snprintf_P(configPath, sizeof(configPath), PSTR("/%s.json"), (const char *)getAppIdName());
  File configFile = LittleFS.open(configPath, "w");
  if (!configFile)
  {
    LOG_SERIAL_PRINTLN(F("Failed to open config file for writing"));
    return false;
  }

  JsonDocument json;
  fillConfigJSON(json, true);
  serializeJson(json, configFile);
  configFile.close();
  return true;
}

bool Application::loadConfig()
{
  bool result = true; // missing file is not an error: default values are already set
  char configPath[32];
  snprintf_P(configPath, sizeof(configPath), PSTR("/%s.json"), (const char *)getAppIdName());
  File configFile = LittleFS.open(configPath, "r");
  if (configFile)
  {
    JsonDocument json;

    DeserializationError deserializeJsonError = deserializeJson(json, configFile);

    // if deserialization failed, then log error and save current config (default values)
    if (deserializeJsonError)
    {
      LOG_SERIAL_PRINTF_P(PSTR("deserializeJson() failed : %s\n"), deserializeJsonError.c_str());
      saveConfig();
      result = false;
    }
    else
    { // otherwise pass it to application
      result = parseConfigJSON(json);
      validateConfig();
    }
    configFile.close();
  }

  return result;
}

// Writes a secret field: real value when saving to file, predefined placeholder when sending to web page
void Application::fillSecret(JsonVariant json, const __FlashStringHelper *key, const char *value, bool forSaveFile)
{
  if (forSaveFile)
    json[key] = value;
  else
    json[key] = (const __FlashStringHelper *)predefPassword;
}

void Application::parseIPField(JsonVariant jv, uint32_t &dest)
{
  if (jv.is<const char *>())
  {
    IPAddress ip;
    if (ip.fromString(jv.as<const char *>()))
      dest = ip;
    else
      dest = 0;
  }
}

bool Application::getLatestUpdateInfo(char *version, char *title /* = nullptr */, char *releaseDate /* = nullptr */, char *summary /* = nullptr */)
{
  if (version)
    version[0] = '\0';
  if (title)
    title[0] = '\0';
  if (releaseDate)
    releaseDate[0] = '\0';
  if (summary)
    summary[0] = '\0';

  WiFiClientSecure clientSecure;
  HTTPClient http;

  clientSecure.setInsecure();
  http.begin(clientSecure, String(F("https://api.github.com/repos/" CUSTOM_APP_MANUFACTURER "/" CUSTOM_APP_MODEL "/releases/latest")));

#ifdef ESP8266
  ESP.wdtFeed(); // fresh 6 s window before TLS handshake
#endif
  int httpCode = http.GET();

#ifdef ESP8266
  ESP.wdtFeed(); // reset before parse phase
#endif

  // check for http error
  if (httpCode != 200)
  {
    http.end();
    return false;
  }

  // httpCode is 200, we can continue
  WiFiClient *stream = http.getStreamPtr();

  // Parse the JSON answer as a stream, without loading it in memory (several KB: assets, author, reactions...).
  // The parsing itself is in ReleaseInfoParser (no Arduino dependency, testable on a PC);
  // this function only provides the characters.

  // readNextChar waits (up to 500ms) briefly for the next byte, allowing the WiFi stack to
  // process incoming TCP segments that haven't arrived yet (fixes premature exit
  // when the receive buffer empties between two TCP segments)
#ifdef ESP8266
  uint16_t byteCount = 0;
#endif
  auto readNextChar = [&](char &c) -> bool
  {
#ifdef ESP8266
    // a run of slow reads can stall up to 500 ms each
    if (++byteCount % 32 == 0)
      ESP.wdtFeed();
#endif
    uint32_t start = millis();
    while (!stream->available())
    {
      if (millis() - start >= 500)
        return false;
      delay(1); // lets the WiFi/lwIP stack process incoming segments
    }
    c = stream->read();
    return true;
  };

  // buffer sizes expected from callers (fillLatestUpdateInfoJson(), updateFirmware()):
  // version[10], title[64], releaseDate[11] ("YYYY-MM-DD"), summary[256]
  ReleaseInfoParser::Targets targets;
  targets.version = version;
  targets.versionSize = 10;
  targets.title = title;
  targets.titleSize = 64;
  targets.releaseDate = releaseDate;
  targets.releaseDateSize = 11;
  targets.summary = summary;
  targets.summarySize = 256;

  ReleaseInfoParser::parse(readNextChar, targets);

  http.end();

  return version && version[0] != '\0';
}

void Application::fillLatestUpdateInfoJson(JsonVariant json, bool forWebPage /* = false */)
{
  json[F("installed_version")] = VERSION;

  // Skip fetching update info if device is in AP mode (no internet connectivity)
  if (WiFi.getMode() & WIFI_AP)
    return;

  char version[10], title[64], releaseDate[11], summary[256];

  if (getLatestUpdateInfo(version, title, releaseDate, summary))
  {
    json[F("latest_version")] = version;
    json[F("title")] = title;
    json[F("release_summary")] = summary;

    char releaseUrl[128];
    snprintf_P(releaseUrl, sizeof(releaseUrl), PSTR("https://github.com/" CUSTOM_APP_MANUFACTURER "/" CUSTOM_APP_MODEL "/releases/tag/%s"), version);
    json[F("release_url")] = releaseUrl;

    if (forWebPage)
      json[F("release_date")] = releaseDate;
    else
      json[F("in_progress")] = (Update.isRunning() ? F("true") : F("false"));
  }
}

// Discards any running/partial update and clears the Updater error state
void Application::resetUpdate()
{
  if (Update.isRunning())
  {
#ifdef ESP8266
    Update.end(); // without evenIfRemaining, end() resets the Updater
#else
    Update.abort(); // on ESP32, end() does not reset when an error is already set
#endif
  }
  Update.clearError();
}

// Returns true if version matches "X.Y.Z" (1 to 3 digits per part, max 11 chars)
bool Application::isValidVersion(const char *version)
{
  if (!version)
    return false;

  uint8_t parts = 1;
  uint8_t digits = 0;
  const char *p = version;

  for (; *p; p++)
  {
    if (*p >= '0' && *p <= '9')
    {
      if (++digits > 3)
        return false;
    }
    else if (*p == '.')
    {
      if (digits == 0 || ++parts > 3)
        return false;
      digits = 0;
    }
    else
      return false;
  }

  return parts == 3 && digits > 0;
}

bool Application::updateFirmware(const char *version, String &retMsg, std::function<void(size_t, size_t)> progressCallback /* = nullptr */)
{
  if (!version || !version[0])
  {
    retMsg = F("No version provided");
    return false;
  }

  char latestVersion[10];
  if (strcmp(version, "latest") == 0)
  {
    if (!getLatestUpdateInfo(latestVersion, nullptr, nullptr, nullptr))
    {
      retMsg = F("Failed to get latest version");
      return false;
    }

    version = latestVersion;
  }

  // only accept a strict "X.Y.Z" format
  if (!isValidVersion(version))
  {
    retMsg = F("Invalid version format");
    return false;
  }

  WiFiClientSecure clientSecure;
  clientSecure.setInsecure();

  char fwUrl[200];
#ifdef ESP8266
  snprintf_P(fwUrl, sizeof(fwUrl), PSTR("https://github.com/" CUSTOM_APP_MANUFACTURER "/" CUSTOM_APP_MODEL "/releases/download/%s/" CUSTOM_APP_MODEL ".%s.bin"), version, version);
#else
  snprintf_P(fwUrl, sizeof(fwUrl), PSTR("https://github.com/" CUSTOM_APP_MANUFACTURER "/" CUSTOM_APP_MODEL "/releases/download/%s/" CUSTOM_APP_MODEL ".esp32.%s.bin"), version, version);
#endif

  LOG_SERIAL_PRINTF_P(PSTR("Trying to Update from URL: %s\n"), fwUrl);

  HTTPClient https;
  https.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  https.begin(clientSecure, fwUrl);

#ifdef ESP8266
  ESP.wdtFeed(); // fresh 6 s window before TLS handshake
#endif
  int httpCode = https.GET();

#ifdef ESP8266
  ESP.wdtFeed(); // fresh 6 s window before stream readiness wait
#endif

  if (httpCode != 200)
  {
    https.end();

    char retMsgBuf[48];
    snprintf_P(retMsgBuf, sizeof(retMsgBuf), PSTR("Failed to download file, httpCode: %d"), httpCode);
    retMsg = retMsgBuf;

    LOG_SERIAL_PRINTLN(retMsg);

    return false;
  }

  // starting here we have a valid httpCode (200)

  // get the stream
  WiFiClient *stream = https.getStreamPtr();
  int contentLength = https.getSize();

  const char *fwName = strrchr(fwUrl, '/');
  fwName = fwName ? fwName + 1 : fwUrl;
  LOG_SERIAL_PRINTF_P(PSTR("Update Start: %s (Online Update)\n"), fwName);

  // without a valid size, Update.begin(-1) would fail with a misleading "Not Enough Space"
  if (contentLength <= 0)
  {
    https.end();
    retMsg = F("Invalid firmware size (no Content-Length)");
    LOG_SERIAL_PRINTLN(retMsg);
    return false;
  }

  // discard any leftover update (e.g. aborted upload) so begin() doesn't fail with "already running"
  resetUpdate();

  if (progressCallback)
    Update.onProgress(progressCallback);

  bool began = Update.begin(contentLength);
  if (began)
  {
    // sometime the stream is not yet ready (no data available yet)
    // and writeStream start by a peek which then fail
    // so wait up to 500ms for the first byte to be available
    uint32_t streamWaitStart = millis();
    while (stream->available() == 0 && millis() - streamWaitStart < 500)
      delay(1); // cooperative yield, lets the WiFi/lwIP stack process incoming segments
    Update.writeStream(*stream);

    Update.end();
  }

  // progress callback is global: don't leak it into a later /fw upload
  Update.onProgress(nullptr);

  https.end();

  bool success = began && !Update.hasError();
  if (success)
    LOG_SERIAL_PRINTLN(F("Update successful"));
  else
  {
    if (Update.hasError())
    {
#ifdef ESP8266
      retMsg = Update.getErrorString();
#else
      retMsg = Update.errorString();
#endif
    }
    else
      retMsg = F("Update.begin failed");
    LOG_SERIAL_PRINTF_P(PSTR("Update failed: %s\n"), retMsg.c_str());
    resetUpdate();
  }

  return success;
}

void Application::mqttPublishStatus(MQTTMan &mqttMan)
{
  JsonDocument json;
  String topic;
  topic.reserve(MQTTMan::baseTopicSize + 5); // base topic + suffix

  topic = mqttMan.getBaseTopic();
  topic += getMqttTopicSuffix();
  fillStatusJSON(json);
  mqttMan.publish(topic.c_str(), json, true);
}

bool Application::mqttPublishUpdate(MQTTMan &mqttMan)
{
  if (!mqttMan.connected())
    return false;

  JsonDocument updateInfo;
  fillLatestUpdateInfoJson(updateInfo);

  String topic = mqttMan.getBaseTopic();
  topic += F("/update");

  return mqttMan.publish(topic.c_str(), updateInfo, true);
}

void Application::init(bool skipExistingConfig)
{
  bool result = true;

  LOG_SERIAL_PRINTF_P(PSTR("Start %s : "), (const char *)getAppIdName());

  setConfigDefaultValues();

  if (!skipExistingConfig)
    result = loadConfig();

  // Execute specific Application Init Code
  result = appInit() && result;

  LOG_SERIAL_PRINTLN(result ? F("OK") : F("FAILED"));
}

void Application::initWebServer(WebServer &server)
{
  char url[16];

  // JSON Status handler
  sprintf_P(url, PSTR("/gs%c"), getAppIdChar());
  server.on(url, HTTP_GET,
            [this, &server]()
            {
              SERVER_KEEPALIVE_FALSE()
              server.sendHeader(F("Cache-Control"), F("no-cache"));
              JsonDocument json;
              fillStatusJSON(json);
              server.setContentLength(measureJson(json));
              server.send(200, F("text/json"), "");
              WiFiClient client = server.client();
              serializeJson(json, client);
            });

  // JSON Config handler
  sprintf_P(url, PSTR("/gc%c"), getAppIdChar());
  server.on(url, HTTP_GET,
            [this, &server]()
            {
              SERVER_KEEPALIVE_FALSE()
              server.sendHeader(F("Cache-Control"), F("no-cache"));
              JsonDocument json;
              fillConfigJSON(json);
              server.setContentLength(measureJson(json));
              server.send(200, F("text/json"), "");
              WiFiClient client = server.client();
              serializeJson(json, client);
            });

  sprintf_P(url, PSTR("/sc%c"), getAppIdChar());
  server.on(url, HTTP_POST,
            [this, &server]()
            {
              // All responses have keep-alive set to false
              SERVER_KEEPALIVE_FALSE()

              // config json are received in POST body (arg("plain"))

              // Deserialize it
              JsonDocument json;
              DeserializationError error = deserializeJson(json, server.arg(F("plain")));
              if (error)
              {
                server.send(400, F("text/html"), F("Malformed JSON"));
                return;
              }

              // Parse it using the application method
              if (!parseConfigJSON(json, true))
              {
                server.send(400, F("text/html"), F("Invalid Configuration"));
                return;
              }
              validateConfig();

              // Save it
              if (!saveConfig())
              {
                server.send(500, F("text/html"), F("Configuration hasn't been saved"));
                return;
              }

              // Everything went fine, Send client answer
              server.send(200);
              _reInit = true;
            });

  // Execute Specific Application Web Server initialization
  appInitWebServer(server);
}

void Application::run()
{
  if (_reInit)
  {
    LOG_SERIAL_PRINTF_P(PSTR("ReStart %s : "), (const char *)getAppIdName());

    if (appInit(true))
      LOG_SERIAL_PRINTLN(F("OK"));
    else
      LOG_SERIAL_PRINTLN(F("FAILED"));

    _reInit = false;
  }

  appRun();
}