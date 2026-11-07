#ifndef ReleaseInfoParser_h
#define ReleaseInfoParser_h

#include <cstddef>
#include <functional>

// Streaming extraction of the few fields WBase needs from the GitHub "latest release" API answer
// (https://api.github.com/repos/<owner>/<repo>/releases/latest).
//
// Why a streaming parser: the answer is several KB (assets, author, reactions...), too much to load
// into an ArduinoJson document on ESP8266. Characters are read one by one and only 4 values are kept.
//
// Why a separate unit: it has no Arduino dependency (only a "read next char" function),
// so it can be compiled and tested on a PC with any JSON text.
//
// What it extracts (top level keys only: "name" inside "author" or "assets" is ignored):
//   "tag_name"     -> version      (empty if it doesn't fit: a truncated version would be a wrong version)
//   "name"         -> title        (the leading version is removed: "3.7.6 New boxes" -> "New boxes")
//   "published_at" -> releaseDate  (truncated to the buffer, e.g. 11 bytes -> "2026-11-06")
//   "body"         -> summary      (truncated to the buffer, cut at the first section heading "##")
//
// JSON handling:
//  - string escapes are decoded: \n \r \t \" \\ \/ \b \f and \uXXXX (written as UTF-8;
//    a lone surrogate half of a \uD83D\uDE00 pair is written as '?')
//  - the escape state is tracked separately from the output buffer, so a value ending with an escaped
//    backslash ("C:\\") or truncated in the middle of an escape sequence never makes the parser miss
//    the closing quote
//  - non-string values (null, numbers, objects...) are skipped: "body": null leaves the summary empty
//    instead of swallowing the next key
namespace ReleaseInfoParser
{
  // returns false when there is no more data (end of stream or timeout)
  using ReadChar = std::function<bool(char &c)>;

  // Each target is optional (nullptr) and receives a '\0'-terminated string; size includes the '\0'.
  struct Targets
  {
    char *version = nullptr;
    size_t versionSize = 0;
    char *title = nullptr;
    size_t titleSize = 0;
    char *releaseDate = nullptr;
    size_t releaseDateSize = 0;
    char *summary = nullptr;
    size_t summarySize = 0;
  };

  // Reads the whole stream. Returns true if a version was found.
  bool parse(const ReadChar &readNextChar, const Targets &targets);
}

#endif
