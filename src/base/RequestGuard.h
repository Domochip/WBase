#ifndef RequestGuard_h
#define RequestGuard_h

// Protection of the web server against requests made by other web sites through the user's browser.
//
// The device has no authentication: any web page opened in a browser on the local network could
// otherwise use that browser to call the device. Two checks, run at the start of every API handler:
//
// 1. Host check (DNS rebinding). A malicious site can make its own domain name resolve to the device's
//    IP address. The browser then treats the device as part of that site: the page can call any URL
//    and read the answers. The request still carries the malicious name in its Host header, so
//    requests whose Host is not one of the device's names are refused.
//    Skipped for requests received through the device's own access point (captive portal: phones open
//    the page under whatever name they probed).
//
// 2. Origin check (CSRF). Browsers add an Origin header to every POST made by a page of another site
//    (form, fetch, XMLHttpRequest). A POST whose Origin is present and is not the device itself is refused.
//    Clients other than browsers (Home Assistant, Jeedom, scripts, curl) send no Origin and are not affected.
//
// The functions below that take only strings have no Arduino dependency and can be tested on a PC.

#include <stddef.h>

namespace RequestGuard
{
  // true if hostHeader (as received, possibly with ":port", any case, possibly with a trailing dot)
  // is one of the names in allowed[], or one of the names of the comma/space separated list extraList.
  // An empty Host header is accepted (HTTP/1.0 clients; browsers always send it).
  bool hostMatches(const char *hostHeader, const char *const allowed[], size_t allowedCount, const char *extraList);

  // true if origin is empty (non-browser client, or same-origin GET), or is "http://<hostHeader>" or
  // "https://<hostHeader>" (case insensitive). "null" (sandboxed page, local file) is refused.
  bool originMatches(const char *origin, const char *hostHeader);
}

#ifndef REQUESTGUARD_HOST_TEST
#include "Application.h" // WebServer type, SERVER_KEEPALIVE_FALSE, WiFi

namespace RequestGuard
{
  // to be called once before server.begin(): asks the server to keep the Origin header
  void setup(WebServer &server);

  // additional host names accepted by the Host check (comma or space separated), e.g. a name given by
  // the router's DNS ("wpalacontrol.lan") or a reverse proxy. The pointed string must stay valid.
  void setExtraHosts(const char *extraHosts);

  // nullptr if the current request is allowed, otherwise the reason (for logs and the 403 answer).
  // Can be used in an upload callback (headers are already parsed when the upload starts).
  const __FlashStringHelper *rejectReason(WebServer &server);

  // true if the current request is allowed; otherwise answers 403 with the reason and returns false.
  // Usage, first line of a handler: if (!RequestGuard::check(server)) return;
  bool check(WebServer &server);
}
#endif

#endif
