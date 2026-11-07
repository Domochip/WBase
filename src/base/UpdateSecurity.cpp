#include "UpdateSecurity.h"
#include "UpdateSigningKey.h"

#ifdef ESP8266
#include <Updater.h>
#include <BearSSLHelpers.h> // BearSSL::PublicKey, HashSHA256, SigningVerifier
#else
#include <Update.h>
#endif

// Signature verification objects, created by armSignatureCheck() and deleted by disarmSignatureCheck().
// Kept as pointers so that nothing is allocated when signing is disabled or while no update is running
// (SigningVerifier reserves BearSSL's second stack, several KB of RAM, for its whole lifetime).
// Update keeps raw pointers to the hash and the verifier between begin() and end(): they must stay alive
// until disarmSignatureCheck() has removed them from Update.
#if defined(ESP8266) && UPDATE_SIGNING_ENABLED
// HashSHA256 and SigningVerifier are polymorphic classes without virtual destructor: they are kept in a
// non-polymorphic holder, so that a single delete runs their destructors without any ambiguity
// (and without -Wdelete-non-virtual-dtor). ~SigningVerifier() releases the BearSSL second stack.
struct SignatureObjects final
{
  BearSSL::HashSHA256 hash;
  BearSSL::SigningVerifier verifier;
  explicit SignatureObjects(BearSSL::PublicKey *key) : verifier(key) {}
};
static BearSSL::PublicKey *signPubKey = nullptr;
static SignatureObjects *signObjects = nullptr;
#endif

bool UpdateSecurity::armSignatureCheck(String &retMsg)
{
#if defined(ESP8266) && UPDATE_SIGNING_ENABLED
  if (!signObjects)
  {
    signPubKey = new BearSSL::PublicKey(UPDATE_SIGNING_PUBKEY_PEM);

    // Fail closed: an unparsable key (e.g. PEM pasted incorrectly) must never mean "no signature required".
    // Update would reject the firmware anyway, but only after the whole download and with a misleading
    // "Signature verification failed": refusing here gives the real cause before downloading anything.
    if (!signPubKey->isRSA() && !signPubKey->isEC())
    {
      delete signPubKey;
      signPubKey = nullptr;
      retMsg = F("Invalid firmware signing key");
      return false;
    }

    signObjects = new SignatureObjects(signPubKey); // the verifier also reserves BearSSL second stack
  }

  // From now on, Update.begin() uses SHA-256 instead of MD5, and Update.end() expects
  // [firmware image][signature][uint32 signature length] and checks the signature with the public key.
  Update.installSignature(&signObjects->hash, &signObjects->verifier);
#else
  // signing disabled (or ESP32): nothing to arm, updates behave as before
  (void)retMsg;
#endif
  return true;
}

void UpdateSecurity::disarmSignatureCheck()
{
#if defined(ESP8266) && UPDATE_SIGNING_ENABLED
  // remove the pointers from Update before deleting the objects they point to
  Update.installSignature(nullptr, nullptr);

  delete signObjects; // ~SigningVerifier() releases BearSSL second stack reference
  signObjects = nullptr;
  delete signPubKey; // after the verifier, which points to it
  signPubKey = nullptr;
#endif
}
