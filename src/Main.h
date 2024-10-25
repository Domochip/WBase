#ifndef Main_h
#define Main_h

#include <Arduino.h>

// Helper macros to convert a define to a string
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

// ----- Should be modified for your application -----
#define APPLICATION1_MANUFACTURER "Domochip" // also used as Github user for OTA
#define APPLICATION1_MODEL "WBase"           // also used as Github repository for OTA
#define APPLICATION1_CLASS WBase
#define VERSION_NUMBER "3.0.0"

#define APPLICATION1_NAME TOSTRING(APPLICATION1_CLASS)     // stringified class name
#define APPLICATION1_HEADER TOSTRING(APPLICATION1_CLASS.h) // calculated header file "{APPLICATION1_NAME}.h"
#define DEFAULT_AP_SSID APPLICATION1_NAME                  // Default Access Point SSID "{APPLICATION1_NAME}{4 digits of ChipID}"
#define DEFAULT_AP_PSK APPLICATION1_NAME "Pass"            // Default Access Point Password "{APPLICATION1_NAME}Pass"

// Control EventSourceMan code (To be used by Application if EventSource server is needed)
#define EVTSRC_ENABLED 1
#define EVTSRC_MAX_CLIENTS 2
#define EVTSRC_KEEPALIVE_ENABLED 1

// Enable developper mode
#define DEVELOPPER_MODE 0

// Log Serial Object
#define LOG_SERIAL Serial
// Choose Log Serial Speed
#define LOG_SERIAL_SPEED 115200

// Log Serial Macros
#ifdef LOG_SERIAL
#define LOG_SERIAL_PRINT(...) LOG_SERIAL.print(__VA_ARGS__)
#define LOG_SERIAL_PRINTLN(...) LOG_SERIAL.println(__VA_ARGS__)
#define LOG_SERIAL_PRINTF(...) LOG_SERIAL.printf(__VA_ARGS__)
#define LOG_SERIAL_PRINTF_P(...) LOG_SERIAL.printf_P(__VA_ARGS__)
#else
#define LOG_SERIAL_PRINT(...)
#define LOG_SERIAL_PRINTLN(...)
#define LOG_SERIAL_PRINTF(...)
#define LOG_SERIAL_PRINTF_P(...)
#endif

// Choose Pin used to boot in Rescue Mode
#define RESCUE_BTN_PIN 2

// Define time to wait for Rescue press (in s)
// #define RESCUE_BUTTON_WAIT 3

// Status LED
// #define STATUS_LED_SETUP pinMode(XX, OUTPUT);pinMode(XX, OUTPUT);
// #define STATUS_LED_OFF digitalWrite(XX, HIGH);digitalWrite(XX, HIGH);
// #define STATUS_LED_ERROR digitalWrite(XX, HIGH);digitalWrite(XX, HIGH);
// #define STATUS_LED_WARNING digitalWrite(XX, HIGH);digitalWrite(XX, HIGH);
// #define STATUS_LED_GOOD digitalWrite(XX, HIGH);digitalWrite(XX, HIGH);

// construct Version text
#if DEVELOPPER_MODE
#define VERSION VERSION_NUMBER "-DEV"
#else
#define VERSION VERSION_NUMBER
#endif

#endif