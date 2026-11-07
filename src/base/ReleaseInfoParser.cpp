#include "ReleaseInfoParser.h"

#include <cstring>
#include <cstdint>

namespace
{
  // Wraps the read function with a one-character push back:
  // when a value turns out not to be a string, its first character must still be seen by the structure tracking.
  class CharSource
  {
  public:
    explicit CharSource(const ReleaseInfoParser::ReadChar &readNextChar) : _read(readNextChar) {}

    bool next(char &c)
    {
      if (_hasPending)
      {
        _hasPending = false;
        c = _pending;
        return true;
      }
      return _read(c);
    }

    void pushBack(char c)
    {
      _pending = c;
      _hasPending = true;
    }

  private:
    const ReleaseInfoParser::ReadChar &_read;
    char _pending = 0;
    bool _hasPending = false;
  };

  // Bounded string builder: never writes past size, always keeps a '\0' terminator.
  struct Output
  {
    char *buf;
    size_t size;
    size_t len = 0;
    bool overflow = false; // something didn't fit
    bool frozen = false;   // stop storing (summary cut at a section heading)

    Output(char *b, size_t s) : buf(b), size(s)
    {
      if (buf && size)
        buf[0] = '\0';
    }

    // appends n bytes only if all of them fit (a UTF-8 sequence is never cut in half)
    void append(const char *bytes, size_t n)
    {
      if (frozen || !buf || size == 0)
        return;
      if (len + n > size - 1)
      {
        overflow = true;
        return;
      }
      memcpy(buf + len, bytes, n);
      len += n;
      buf[len] = '\0';
    }

    void append(char c) { append(&c, 1); }

    bool endsWith(const char *s) const
    {
      size_t n = strlen(s);
      return len >= n && memcmp(buf + len - n, s, n) == 0;
    }

    void cut(size_t n)
    {
      len -= n;
      buf[len] = '\0';
    }
  };

  int hexValue(char c)
  {
    if (c >= '0' && c <= '9')
      return c - '0';
    if (c >= 'a' && c <= 'f')
      return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
      return c - 'A' + 10;
    return -1;
  }

  // writes a code point of the Basic Multilingual Plane as UTF-8
  void appendUtf8(Output &out, uint16_t cp)
  {
    char bytes[3];
    if (cp >= 0xD800 && cp <= 0xDFFF)
      out.append('?'); // half of a surrogate pair (emoji...): not combined, replaced
    else if (cp < 0x80)
      out.append((char)cp);
    else if (cp < 0x800)
    {
      bytes[0] = (char)(0xC0 | (cp >> 6));
      bytes[1] = (char)(0x80 | (cp & 0x3F));
      out.append(bytes, 2);
    }
    else
    {
      bytes[0] = (char)(0xE0 | (cp >> 12));
      bytes[1] = (char)(0x80 | ((cp >> 6) & 0x3F));
      bytes[2] = (char)(0x80 | (cp & 0x3F));
      out.append(bytes, 3);
    }
  }

  enum class Field
  {
    None,
    Version,
    Title,
    ReleaseDate,
    Summary
  };

  // Reads a JSON string value (the opening quote is already consumed) into out, decoding escapes.
  // Always reads up to the closing quote, even when out is full, so the stream stays in sync.
  void readStringValue(CharSource &src, Output &out, Field field)
  {
    bool escaped = false;
    char c;
    while (src.next(c))
    {
      if (escaped)
      {
        escaped = false;
        switch (c)
        {
        case 'n':
          out.append('\n');
          break;
        case 'r':
          out.append('\r');
          break;
        case 't':
          out.append('\t');
          break;
        case 'b':
        case 'f':
          out.append(' ');
          break;
        case 'u':
        {
          uint16_t cp = 0;
          int digits = 0;
          char h;
          while (digits < 4 && src.next(h))
          {
            int v = hexValue(h);
            if (v < 0)
            {
              // malformed escape: give the character back (it may be the closing quote)
              src.pushBack(h);
              break;
            }
            cp = (cp << 4) | v;
            digits++;
          }
          if (digits == 4)
            appendUtf8(out, cp);
          else
            out.append('?');
          break;
        }
        default: // \" \\ \/ and anything unexpected: the character itself
          out.append(c);
          break;
        }
      }
      else if (c == '\\')
        escaped = true;
      else if (c == '"')
        return; // closing quote
      else
        out.append(c);

      // summary: keep only the introduction, stop at the first section heading
      // (both line ending styles: releases edited on github.com use \r\n, others \n)
      if (field == Field::Summary && !out.frozen)
      {
        if (out.endsWith("\r\n\r\n##"))
        {
          out.cut(6);
          out.frozen = true;
        }
        else if (out.endsWith("\n\n##"))
        {
          out.cut(4);
          out.frozen = true;
        }
      }
    }
  }

  // "3.7.6 New boxes" -> "New boxes"; a title without space is kept as it is
  void removeVersionPrefix(char *title)
  {
    char *space = strchr(title, ' ');
    if (space)
      memmove(title, space + 1, strlen(space + 1) + 1);
  }
}

bool ReleaseInfoParser::parse(const ReadChar &readNextChar, const Targets &targets)
{
  if (targets.version && targets.versionSize)
    targets.version[0] = '\0';
  if (targets.title && targets.titleSize)
    targets.title[0] = '\0';
  if (targets.releaseDate && targets.releaseDateSize)
    targets.releaseDate[0] = '\0';
  if (targets.summary && targets.summarySize)
    targets.summary[0] = '\0';

  CharSource src(readNextChar);

  int depth = 0;              // nesting level of objects/arrays: the release object itself is depth 1
  bool inString = false;      // inside a string that is not a value we extract
  bool escaped = false;       // previous character inside that string was a backslash
  char key[16] = {0};         // last string read at depth 1 (candidate key)
  size_t keyLen = 0;
  bool keyTooLong = false;    // longer than any key we look for
  bool collectingKey = false; // the current string is at depth 1
  bool keyCandidate = false;  // a depth 1 string just ended: if ':' follows, it was a key

  char c;
  while (src.next(c))
  {
    if (inString)
    {
      if (escaped)
        escaped = false;
      else if (c == '\\')
        escaped = true;
      else if (c == '"')
      {
        inString = false;
        keyCandidate = collectingKey;
        continue;
      }

      if (collectingKey)
      {
        if (keyLen < sizeof(key) - 1)
          key[keyLen++] = c;
        else
          keyTooLong = true;
      }
      continue;
    }

    switch (c)
    {
    case '"':
      inString = true;
      escaped = false;
      collectingKey = (depth == 1);
      keyLen = 0;
      keyTooLong = false;
      keyCandidate = false;
      break;

    case '{':
    case '[':
      depth++;
      keyCandidate = false;
      break;

    case '}':
    case ']':
      if (depth > 0)
        depth--;
      keyCandidate = false;
      break;

    case ' ':
    case '\t':
    case '\r':
    case '\n':
      break; // whitespace between a key and ':' keeps the candidate

    case ':':
    {
      if (!keyCandidate || keyTooLong)
        break;
      keyCandidate = false;
      key[keyLen] = '\0';

      Field field = Field::None;
      char *buf = nullptr;
      size_t size = 0;
      if (strcmp(key, "tag_name") == 0)
        field = Field::Version, buf = targets.version, size = targets.versionSize;
      else if (strcmp(key, "name") == 0)
        field = Field::Title, buf = targets.title, size = targets.titleSize;
      else if (strcmp(key, "published_at") == 0)
        field = Field::ReleaseDate, buf = targets.releaseDate, size = targets.releaseDateSize;
      else if (strcmp(key, "body") == 0)
        field = Field::Summary, buf = targets.summary, size = targets.summarySize;

      if (field == Field::None || !buf || size == 0)
        break;

      // skip whitespace before the value
      char v;
      while (src.next(v) && (v == ' ' || v == '\t' || v == '\r' || v == '\n'))
        ;

      // not a string (null, number, object...): leave the target empty and let the
      // main loop process that character (it may open an object or an array)
      if (v != '"')
      {
        src.pushBack(v);
        break;
      }

      Output out(buf, size);
      readStringValue(src, out, field);

      if (field == Field::Version && out.overflow)
        buf[0] = '\0'; // a truncated version number would be a wrong version number
      else if (field == Field::Title)
        removeVersionPrefix(buf);
      break;
    }

    default:
      keyCandidate = false;
      break;
    }
  }

  return targets.version && targets.version[0] != '\0';
}
