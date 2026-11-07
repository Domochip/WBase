#ifndef UpdateSecurity_h
#define UpdateSecurity_h

#include <Arduino.h>

// Optional firmware signature check (ESP8266 only).
// When enabled (UPDATE_SIGNING_ENABLED and a public key in UpdateSigningKey.h), Update only accepts
// firmwares signed with the project's private key, whatever their origin: online update, MQTT/Home Assistant
// update, or web upload on /fw. Disabled by default: nothing changes until the key is set.
//
// Usage (see Application::updateFirmware() and the /fw handler in Core.cpp):
//    if (!UpdateSecurity::armSignatureCheck(retMsg)) return false;  // before Update.begin()
//    Update.begin(...); Update.write...; Update.end();               // end() verifies the signature
//    UpdateSecurity::disarmSignatureCheck();                         // after end() or after an abort

namespace UpdateSecurity
{
  // Require a valid signature for the next firmware written through Update (ESP8266 only).
  // Must be called before Update.begin(): Update chooses between MD5 and signature verification in begin().
  // Returns false (and sets retMsg) if signing is enabled but can't be armed: the update must be refused.
  // Returns true when signing is disabled (nothing armed) or correctly armed.
  bool armSignatureCheck(String &retMsg);

  // Release signature verification resources once the update ended or was aborted.
  // Safe to call several times, and when nothing was armed.
  // Releasing matters on ESP8266: the verifier holds BearSSL's second stack (several KB of RAM).
  void disarmSignatureCheck();
}

#endif
