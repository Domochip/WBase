#include "RequestGuard.h"

#include <string.h>
#include <ctype.h>

namespace
{
  // length of the host part of a Host header: without ":port" and without a trailing dot
  size_t hostLength(const char *host)
  {
    size_t len = strlen(host);
    const char *colon = strchr(host, ':');
    if (colon)
      len = colon - host;
    if (len > 0 && host[len - 1] == '.')
      len--;
    return len;
  }

  bool equalsIgnoreCase(const char *a, size_t aLen, const char *b, size_t bLen)
  {
    if (aLen != bLen)
      return false;
    for (size_t i = 0; i < aLen; i++)
      if (tolower((unsigned char)a[i]) != tolower((unsigned char)b[i]))
        return false;
    return true;
  }
}

bool RequestGuard::hostMatches(const char *hostHeader, const char *const allowed[], size_t allowedCount, const char *extraList)
{
  if (!hostHeader || !hostHeader[0])
    return true;

  size_t len = hostLength(hostHeader);
  if (len == 0)
    return false;

  for (size_t i = 0; i < allowedCount; i++)
    if (allowed[i] && allowed[i][0] && equalsIgnoreCase(hostHeader, len, allowed[i], strlen(allowed[i])))
      return true;

  // extra names: "name1, name2 name3"
  if (extraList)
  {
    const char *p = extraList;
    while (*p)
    {
      while (*p == ',' || *p == ' ')
        p++;
      const char *start = p;
      while (*p && *p != ',' && *p != ' ')
        p++;
      size_t nameLen = p - start;
      if (nameLen > 0 && start[nameLen - 1] == '.')
        nameLen--;
      if (nameLen > 0 && equalsIgnoreCase(hostHeader, len, start, nameLen))
        return true;
    }
  }

  return false;
}

bool RequestGuard::originMatches(const char *origin, const char *hostHeader)
{
  if (!origin || !origin[0])
    return true;
  if (!hostHeader || !hostHeader[0])
    return false;

  const char *rest;
  if (strncasecmp(origin, "http://", 7) == 0)
    rest = origin + 7;
  else if (strncasecmp(origin, "https://", 8) == 0)
    rest = origin + 8;
  else
    return false; // "null" or another scheme

  // Origin and Host both carry the port only when it is not the default one, so they compare as is
  return strcasecmp(rest, hostHeader) == 0;
}

#ifndef REQUESTGUARD_HOST_TEST

namespace
{
  const char *g_extraHosts = nullptr;
}

void RequestGuard::setup(WebServer &server)
{
  static const char *headerKeys[] = {"Origin"};
  server.collectHeaders(headerKeys, 1);
}

void RequestGuard::setExtraHosts(const char *extraHosts)
{
  g_extraHosts = extraHosts;
}

const __FlashStringHelper *RequestGuard::rejectReason(WebServer &server)
{
  String host = server.hostHeader();

  // 1. Host check, except for requests received through the device's own access point
  bool viaAP = (WiFi.getMode() & WIFI_AP) && server.client().localIP() == WiFi.softAPIP();
  if (!viaAP)
  {
    String ip = WiFi.localIP().toString();
    String apIp = WiFi.softAPIP().toString();
#ifdef ESP8266
    String hostname = WiFi.hostname();
#else
    String hostname = WiFi.getHostname();
#endif
    String hostnameLocal = hostname + F(".local");
    const char *allowed[] = {ip.c_str(), apIp.c_str(), hostname.c_str(), hostnameLocal.c_str(),
                             CUSTOM_APP_MODEL, CUSTOM_APP_MODEL ".local"}; // mDNS name, see WifiMan::appInit

    if (!hostMatches(host.c_str(), allowed, sizeof(allowed) / sizeof(allowed[0]), g_extraHosts))
      return F("unknown host name. Use the device IP address, or add this name in System configuration > Additional host names");
  }

  // 2. Origin check, for requests that change something (everything but GET and HEAD)
  if (server.method() != HTTP_GET && server.method() != HTTP_HEAD)
  {
    String origin = server.header(F("Origin"));
    if (!originMatches(origin.c_str(), host.c_str()))
      return F("request sent by another web site (Origin header)");
  }

  return nullptr;
}

bool RequestGuard::check(WebServer &server)
{
  const __FlashStringHelper *reason = rejectReason(server);
  if (!reason)
    return true;

  String msg = F("Forbidden: ");
  msg += reason;
  LOG_SERIAL_PRINTF_P(PSTR("Request refused (%s, Host: %s): %s\n"), server.uri().c_str(), server.hostHeader().c_str(), String(reason).c_str());

  SERVER_KEEPALIVE_FALSE()
  server.send(403, F("text/plain"), msg);
  return false;
}

#endif
