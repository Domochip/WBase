#ifndef UpdateSigningKey_h
#define UpdateSigningKey_h

#include <Arduino.h>

// Public key used to verify firmware signatures (ESP8266 only).
//
// Why sign firmwares:
// The online update doesn't check GitHub's certificate (setInsecure), and the web upload page (/fw) accepts
// any file. A signature check in the device is independent of how the file arrives (online, MQTT, /fw, any
// server): only firmwares signed with the project's private key are installed.
//
// How it works (ESP8266 Arduino core "signed updates", see doc/ota_updates/readme.rst in the core):
//  - at release time, tools/signing.py (shipped with the ESP8266 core) appends to the .bin:
//      [firmware image][RSA signature of SHA-256(image)][uint32 little-endian signature length]
//  - on the device, UpdateSecurity::armSignatureCheck() installs a BearSSL verifier into Update before
//    Update.begin(); Update.end() then hashes the image, checks the signature with the public key below,
//    and fails with UPDATE_ERROR_SIGN ("Signature verification failed") if it doesn't match.
//  - firmwares running without the verifier (older versions, or UPDATE_SIGNING_ENABLED 0) ignore the trailing
//    signature, so a signed .bin can be installed on any device: enabling signing needs no special transition release.
//
// Setup for official builds (once):
//  1. generate the key pair and keep private.key secret
//     (store it as the GitHub Actions secret FIRMWARE_SIGNING_KEY, never in the repository):
//       openssl genrsa -out private.key 2048
//       openssl rsa -in private.key -outform PEM -pubout -out public.key
//  2. paste public.key between the PEM markers below and set UPDATE_SIGNING_ENABLED to 1
//  3. sign the ESP8266 .bin files in the release workflow, after "pio run" (see the pull request description)
//  Losing private.key means devices with this public key can only be updated through USB: keep a backup.
//
// With UPDATE_SIGNING_ENABLED set to 1, the device refuses any firmware (web upload,
// online update, MQTT update) that is not signed with the matching private key.
// Self-built firmwares can still be flashed through USB.
//
// With UPDATE_SIGNING_ENABLED set to 0, unsigned firmwares are accepted (previous behaviour).
#define UPDATE_SIGNING_ENABLED 0

static const char UPDATE_SIGNING_PUBKEY_PEM[] PROGMEM = R"PEM(
-----BEGIN PUBLIC KEY-----
-----END PUBLIC KEY-----
)PEM";

#endif
