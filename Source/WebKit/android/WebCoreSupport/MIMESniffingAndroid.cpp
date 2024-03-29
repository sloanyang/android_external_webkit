// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Detecting mime types is a tricky business because we need to balance
// compatibility concerns with security issues.  Here is a survey of how other
// browsers behave and then a description of how we intend to behave.
//
// HTML payload, no Content-Type header:
// * IE 7: Render as HTML
// * Firefox 2: Render as HTML
// * Safari 3: Render as HTML
// * Opera 9: Render as HTML
//
// Here the choice seems clear:
// => Chrome: Render as HTML
//
// HTML payload, Content-Type: "text/plain":
// * IE 7: Render as HTML
// * Firefox 2: Render as text
// * Safari 3: Render as text (Note: Safari will Render as HTML if the URL
//                                   has an HTML extension)
// * Opera 9: Render as text
//
// Here we choose to follow the majority (and break some compatibility with IE).
// Many folks dislike IE's behavior here.
// => Chrome: Render as text
// We generalize this as follows.  If the Content-Type header is text/plain
// we won't detect dangerous mime types (those that can execute script).
//
// HTML payload, Content-Type: "application/octet-stream":
// * IE 7: Render as HTML
// * Firefox 2: Download as application/octet-stream
// * Safari 3: Render as HTML
// * Opera 9: Render as HTML
//
// We follow Firefox.
// => Chrome: Download as application/octet-stream
// One factor in this decision is that IIS 4 and 5 will send
// application/octet-stream for .xhtml files (because they don't recognize
// the extension).  We did some experiments and it looks like this doesn't occur
// very often on the web.  We choose the more secure option.
//
// GIF payload, no Content-Type header:
// * IE 7: Render as GIF
// * Firefox 2: Render as GIF
// * Safari 3: Download as Unknown (Note: Safari will Render as GIF if the
//                                        URL has an GIF extension)
// * Opera 9: Render as GIF
//
// The choice is clear.
// => Chrome: Render as GIF
// Once we decide to render HTML without a Content-Type header, there isn't much
// reason not to render GIFs.
//
// GIF payload, Content-Type: "text/plain":
// * IE 7: Render as GIF
// * Firefox 2: Download as application/octet-stream (Note: Firefox will
//                              Download as GIF if the URL has an GIF extension)
// * Safari 3: Download as Unknown (Note: Safari will Render as GIF if the
//                                        URL has an GIF extension)
// * Opera 9: Render as GIF
//
// Displaying as text/plain makes little sense as the content will look like
// gibberish.  Here, we could change our minds and download.
// => Chrome: Render as GIF
//
// GIF payload, Content-Type: "application/octet-stream":
// * IE 7: Render as GIF
// * Firefox 2: Download as application/octet-stream (Note: Firefox will
//                              Download as GIF if the URL has an GIF extension)
// * Safari 3: Download as Unknown (Note: Safari will Render as GIF if the
//                                        URL has an GIF extension)
// * Opera 9: Render as GIF
//
// We used to render as GIF here, but the problem is that some sites want to
// trigger downloads by sending application/octet-stream (even though they
// should be sending Content-Disposition: attachment).  Although it is safe
// to render as GIF from a security perspective, we actually get better
// compatibility if we don't sniff from application/octet stream at all.
// => Chrome: Download as application/octet-stream
//
// XHTML payload, Content-Type: "text/xml":
// * IE 7: Render as XML
// * Firefox 2: Render as HTML
// * Safari 3: Render as HTML
// * Opera 9: Render as HTML
// The layout tests rely on us rendering this as HTML.
// But we're conservative in XHTML detection, as this runs afoul of the
// "don't detect dangerous mime types" rule.
//
// Note that our definition of HTML payload is much stricter than IE's
// definition and roughly the same as Firefox's definition.

//this file copy from external/chromium/net/base/Mime_sniffer.cc


#include "MIMESniffingAndroid.h"
#include "ChromiumIncludes.h"


namespace android {

// The number of content bytes we need to use all our magic numbers.  Feel free
// to increase this number if you add a longer magic number.
static const size_t kBytesRequiredForMagic = 42;

struct MagicNumber {
  const char* mime_type;
  const char* magic;
  size_t magic_len;
  bool is_string;
};

#define MAGIC_NUMBER(mime_type, magic) \
  { (mime_type), (magic), sizeof(magic)-1, false },

// Magic strings are case insensitive and must not include '\0' characters
#define MAGIC_STRING(mime_type, magic) \
  { (mime_type), (magic), sizeof(magic)-1, true },

static const MagicNumber kMagicNumbers[] = {
  // Source: HTML 5 specification
  MAGIC_NUMBER("application/pdf", "%PDF-")
  MAGIC_NUMBER("application/postscript", "%!PS-Adobe-")
  MAGIC_NUMBER("image/gif", "GIF87a")
  MAGIC_NUMBER("image/gif", "GIF89a")
  MAGIC_NUMBER("image/png", "\x89" "PNG\x0D\x0A\x1A\x0A")
  MAGIC_NUMBER("image/jpeg", "\xFF\xD8\xFF")
  MAGIC_NUMBER("image/bmp", "BM")
  // Source: Mozilla
  MAGIC_NUMBER("text/plain", "#!")  // Script
  MAGIC_NUMBER("text/plain", "%!")  // Script, similar to PS
  MAGIC_NUMBER("text/plain", "From")
  MAGIC_NUMBER("text/plain", ">From")
  // Chrome specific
  MAGIC_NUMBER("application/x-gzip", "\x1F\x8B\x08")
  MAGIC_NUMBER("audio/x-pn-realaudio", "\x2E\x52\x4D\x46")
  MAGIC_NUMBER("video/x-ms-asf",
      "\x30\x26\xB2\x75\x8E\x66\xCF\x11\xA6\xD9\x00\xAA\x00\x62\xCE\x6C")
  MAGIC_NUMBER("image/tiff", "I I")
  MAGIC_NUMBER("image/tiff", "II*")
  MAGIC_NUMBER("image/tiff", "MM\x00*")
  MAGIC_NUMBER("audio/mpeg", "ID3")
  MAGIC_NUMBER("image/webp", "RIFF....WEBPVP8 ")
  MAGIC_NUMBER("video/webm", "\x1A\x45\xDF\xA3")
  // TODO(abarth): we don't handle partial byte matches yet
  // MAGIC_NUMBER("video/mpeg", "\x00\x00\x01\xB")
  // MAGIC_NUMBER("audio/mpeg", "\xFF\xE")
  // MAGIC_NUMBER("audio/mpeg", "\xFF\xF")
  MAGIC_NUMBER("application/zip", "PK\x03\x04")
  MAGIC_NUMBER("application/x-rar-compressed", "Rar!\x1A\x07\x00")
  MAGIC_NUMBER("application/x-msmetafile", "\xD7\xCD\xC6\x9A")
  MAGIC_NUMBER("application/octet-stream", "MZ")  // EXE
  // Sniffing for Flash:
  //
  //   MAGIC_NUMBER("application/x-shockwave-flash", "CWS")
  //   MAGIC_NUMBER("application/x-shockwave-flash", "FLV")
  //   MAGIC_NUMBER("application/x-shockwave-flash", "FWS")
  //
  // Including these magic number for Flash is a trade off.
  //
  // Pros:
  //   * Flash is an important and popular file format
  //
  // Cons:
  //   * These patterns are fairly weak
  //   * If we mistakenly decide something is Flash, we will execute it
  //     in the origin of an unsuspecting site.  This could be a security
  //     vulnerability if the site allows users to upload content.
  //
  // On balance, we do not include these patterns.
};

// Our HTML sniffer differs slightly from Mozilla.  For example, Mozilla will
// decide that a document that begins "<!DOCTYPE SOAP-ENV:Envelope PUBLIC " is
// HTML, but we will not.

#define MAGIC_HTML_TAG(tag) \
  MAGIC_STRING("text/html", "<" tag)

static const MagicNumber kSniffableTags[] = {
  // XML processing directive.  Although this is not an HTML mime type, we sniff
  // for this in the HTML phase because text/xml is just as powerful as HTML and
  // we want to leverage our white space skipping technology.
  MAGIC_NUMBER("text/xml", "<?xml")  // Mozilla
  // DOCTYPEs
  MAGIC_HTML_TAG("!DOCTYPE html")  // HTML5 spec
  // Sniffable tags, ordered by how often they occur in sniffable documents.
  MAGIC_HTML_TAG("script")  // HTML5 spec, Mozilla
  MAGIC_HTML_TAG("html")  // HTML5 spec, Mozilla
  MAGIC_HTML_TAG("!--")
  MAGIC_HTML_TAG("head")  // HTML5 spec, Mozilla
  MAGIC_HTML_TAG("iframe")  // Mozilla
  MAGIC_HTML_TAG("h1")  // Mozilla
  MAGIC_HTML_TAG("div")  // Mozilla
  MAGIC_HTML_TAG("font")  // Mozilla
  MAGIC_HTML_TAG("table")  // Mozilla
  MAGIC_HTML_TAG("a")  // Mozilla
  MAGIC_HTML_TAG("style")  // Mozilla
  MAGIC_HTML_TAG("title")  // Mozilla
  MAGIC_HTML_TAG("b")  // Mozilla
  MAGIC_HTML_TAG("body")  // Mozilla
  MAGIC_HTML_TAG("br")
  MAGIC_HTML_TAG("p")  // Mozilla
};


// Compare content header to a magic number where magic_entry can contain '.'
// for single character of anything, allowing some bytes to be skipped.
static bool MagicCmp(const char* magic_entry, const char* content, size_t len) {
  while (len) {
    if ((*magic_entry != '.') && (*magic_entry != *content))
      return false;
    ++magic_entry;
    ++content;
    --len;
  }
  return true;
}

static bool MatchMagicNumber(const char* content, size_t size,
                             const MagicNumber* magic_entry,
                             std::string* result) {
  const size_t len = magic_entry->magic_len;

  // Keep kBytesRequiredForMagic honest.
  DCHECK_LE(len, kBytesRequiredForMagic);

  // To compare with magic strings, we need to compute strlen(content), but
  // content might not actually have a null terminator.  In that case, we
  // pretend the length is content_size.
  const char* end =
      static_cast<const char*>(memchr(content, '\0', size));
  const size_t content_strlen =
      (end != NULL) ? static_cast<size_t>(end - content) : size;

  bool match = false;
  if (magic_entry->is_string) {
    if (content_strlen >= len) {
      // String comparisons are case-insensitive
      match = (base::strncasecmp(magic_entry->magic, content, len) == 0);
    }
  } else {
    if (size >= len)
      match = MagicCmp(magic_entry->magic, content, len);
  }

  if (match) {
    result->assign(magic_entry->mime_type);
    return true;
  }
  return false;
}

static bool CheckForMagicNumbers(const char* content, size_t size,
                                 const MagicNumber* magic, size_t magic_len,
                                 std::string* result) {
  for (size_t i = 0; i < magic_len; ++i) {
    if (MatchMagicNumber(content, size, &(magic[i]), result)) {
      return true;
    }
  }
  return false;
}

// Truncates |size| to |max_size| and returns true if |size| is at least
// |max_size|.
static bool TruncateSize(const size_t max_size, size_t* size) {
  // Keep kMaxBytesToSniff honest.
  DCHECK_LE(static_cast<int>(max_size), kMaxBytesToSniff);

  if (*size >= max_size) {
    *size = max_size;
    return true;
  }
  return false;
}

// Returns true and sets result if the content appears to be HTML.
// Clears have_enough_content if more data could possibly change the result.
static bool SniffForHTML(const char* content,
                         size_t size,
                         bool* have_enough_content,
                         std::string* result) {
  // For HTML, we are willing to consider up to 512 bytes. This may be overly
  // conservative as IE only considers 256.
  *have_enough_content &= TruncateSize(512, &size);

  // We adopt a strategy similar to that used by Mozilla to sniff HTML tags,
  // but with some modifications to better match the HTML5 spec.
  const char* const end = content + size;
  const char* pos;
  for (pos = content; pos < end; ++pos) {
    if (!IsAsciiWhitespace(*pos))
      break;
  }
  
  // |pos| now points to first non-whitespace character (or at end).
  return CheckForMagicNumbers(pos, end - pos,
                              kSniffableTags, arraysize(kSniffableTags),
                               result);
}

// Returns true and sets result if the content matches any of kMagicNumbers.
// Clears have_enough_content if more data could possibly change the result.
static bool SniffForMagicNumbers(const char* content,
                                 size_t size,
                                 bool* have_enough_content,
                                 std::string* result) {
  *have_enough_content &= TruncateSize(kBytesRequiredForMagic, &size);

  
  return CheckForMagicNumbers(content, size,
                              kMagicNumbers, arraysize(kMagicNumbers),
                               result);
}

// Byte order marks
static const MagicNumber kMagicXML[] = {
  // We want to be very conservative in interpreting text/xml content as
  // XHTML -- we just want to sniff enough to make unit tests pass.
  // So we match explicitly on this, and don't match other ways of writing
  // it in semantically-equivalent ways.
  MAGIC_STRING("application/xhtml+xml",
               "<html xmlns=\"http://www.w3.org/1999/xhtml\"")
  MAGIC_STRING("application/atom+xml", "<feed")
  MAGIC_STRING("application/rss+xml", "<rss")  // UTF-8
};

// Returns true and sets result if the content appears to contain XHTML or a
// feed.
// Clears have_enough_content if more data could possibly change the result.
//
// TODO(evanm): this is similar but more conservative than what Safari does,
// while HTML5 has a different recommendation -- what should we do?
// TODO(evanm): this is incorrect for documents whose encoding isn't a superset
// of ASCII -- do we care?
static bool SniffXML(const char* content,
                     size_t size,
                     bool* have_enough_content,
                     std::string* result) {
  // We allow at most 300 bytes of content before we expect the opening tag.
  *have_enough_content &= TruncateSize(300, &size);
  const char* pos = content;
  const char* const end = content + size;

  
  // This loop iterates through tag-looking offsets in the file.
  // We want to skip XML processing instructions (of the form "<?xml ...")
  // and stop at the first "plain" tag, then make a decision on the mime-type
  // based on the name (or possibly attributes) of that tag.
  const int kMaxTagIterations = 5;
  for (int i = 0; i < kMaxTagIterations && pos < end; ++i) {
    pos = reinterpret_cast<const char*>(memchr(pos, '<', end - pos));
    if (!pos)
      return false;

    if (base::strncasecmp(pos, "<?xml", sizeof("<?xml")-1) == 0) {
      // Skip XML declarations.
      ++pos;
      continue;
    } else if (base::strncasecmp(pos, "<!DOCTYPE",
                                 sizeof("<!DOCTYPE")-1) == 0) {
      // Skip DOCTYPE declarations.
      ++pos;
      continue;
    }

    if (CheckForMagicNumbers(pos, end - pos,
                             kMagicXML, arraysize(kMagicXML),
                              result))
      return true;

    // TODO(evanm): handle RSS 1.0, which is an RDF format and more difficult
    // to identify.

    // If we get here, we've hit an initial tag that hasn't matched one of the
    // above tests.  Abort.
    return true;
  }

  // We iterated too far without finding a start tag.
  // If we have more content to look at, we aren't going to change our mind by
  // seeing more bytes from the network.
  return pos < end;
}

// Byte order marks
static const MagicNumber kByteOrderMark[] = {
  MAGIC_NUMBER("text/plain", "\xFE\xFF")  // UTF-16BE
  MAGIC_NUMBER("text/plain", "\xFF\xFE")  // UTF-16LE
  MAGIC_NUMBER("text/plain", "\xEF\xBB\xBF")  // UTF-8
};

// Whether a given byte looks like it might be part of binary content.
// Source: HTML5 spec
static char kByteLooksBinary[] = {
  1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 0, 0, 1, 1,  // 0x00 - 0x0F
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1,  // 0x10 - 0x1F
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 0x20 - 0x2F
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 0x30 - 0x3F
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 0x40 - 0x4F
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 0x50 - 0x5F
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 0x60 - 0x6F
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 0x70 - 0x7F
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 0x80 - 0x8F
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 0x90 - 0x9F
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 0xA0 - 0xAF
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 0xB0 - 0xBF
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 0xC0 - 0xCF
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 0xD0 - 0xDF
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 0xE0 - 0xEF
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 0xF0 - 0xFF
};

// Returns true and sets result to "application/octet-stream" if the content
// appears to be binary data. Otherwise, returns false and sets "text/plain".
// Clears have_enough_content if more data could possibly change the result.
static bool SniffBinary(const char* content,
                        size_t size,
                        bool* have_enough_content,
                        std::string* result) {
  // There is no concensus about exactly how to sniff for binary content.
  // * IE 7: Don't sniff for binary looking bytes, but trust the file extension.
  // * Firefox 3.5: Sniff first 4096 bytes for a binary looking byte.
  // Here, we side with FF, but with a smaller buffer. This size was chosen
  // because it is small enough to comfortably fit into a single packet (after
  // allowing for headers) and yet large enough to account for binary formats
  // that have a significant amount of ASCII at the beginning (crbug.com/15314).
  const bool is_truncated = TruncateSize(kMaxBytesToSniff, &size);

  
  // First, we look for a BOM.
  std::string unused;
  if (CheckForMagicNumbers(content, size,
                           kByteOrderMark, arraysize(kByteOrderMark),
                           /*counter,*/ &unused)) {
    // If there is BOM, we think the buffer is not binary.
    result->assign("text/plain");
    return false;
  }

  // Next we look to see if any of the bytes "look binary."
  for (size_t i = 0; i < size; ++i) {
    // If we a see a binary-looking byte, we think the content is binary.
    if (kByteLooksBinary[static_cast<unsigned char>(content[i])]) {
      result->assign("application/octet-stream");
      return true;
    }
  }

  // No evidence either way. Default to non-binary and, if truncated, clear
  // have_enough_content because there could be a binary looking byte in the
  // truncated data.
  *have_enough_content &= is_truncated;
  result->assign("text/plain");
  return false;
}

static bool IsUnknownMimeType(const std::string& mime_type) {
  // TODO(tc): Maybe reuse some code in net/http/http_response_headers.* here.
  // If we do, please be careful not to alter the semantics at all.
  static const char* kUnknownMimeTypes[] = {
    // Empty mime types are as unknown as they get.
    "",
    // The unknown/unknown type is popular and uninformative
    "unknown/unknown",
    // The second most popular unknown mime type is application/unknown
    "application/unknown",
    // Firefox rejects a mime type if it is exactly */*
    "*/*",
  };


  for (size_t i = 0; i < arraysize(kUnknownMimeTypes); ++i) {
    if (mime_type == kUnknownMimeTypes[i]) {
      return true;
    }
  }
  if (mime_type.find('/') == std::string::npos) {
    // Firefox rejects a mime type if it does not contain a slash
    return true;
  }
  return false;
}

// Returns true and sets result if the content appears to be a crx (chrome
// extension) file.
// Clears have_enough_content if more data could possibly change the result.
static bool SniffCRX(const char* content,
                     size_t size,
                     const GURL& url,
                     const std::string& type_hint,
                     bool* have_enough_content,
                     std::string* result) {

  // Technically, the crx magic number is just Cr24, but the bytes after that
  // are a version number which changes infrequently. Including it in the
  // sniffing gives us less room for error. If the version number ever changes,
  // we can just add an entry to this list.
  //
  // TODO(aa): If we ever have another magic number, we'll want to pass a
  // histogram into CheckForMagicNumbers(), below, to see which one matched.
  static const struct MagicNumber kCRXMagicNumbers[] = {
    MAGIC_NUMBER("application/x-chrome-extension", "Cr24\x02\x00\x00\x00")
  };

  // Only consider files that have the extension ".crx".
  static const char kCRXExtension[] = ".crx";
  // Ignore null by subtracting 1.
  static const int kExtensionLength = arraysize(kCRXExtension) - 1;
  if (url.path().rfind(kCRXExtension, std::string::npos, kExtensionLength) ==
      url.path().size() - kExtensionLength) {
    //counter->Add(1);
  } else {
    return false;
  }

  *have_enough_content &= TruncateSize(kBytesRequiredForMagic, &size);
  if (CheckForMagicNumbers(content, size,
                           kCRXMagicNumbers, arraysize(kCRXMagicNumbers),
                            result)) {
    //counter->Add(2);
  } else {
    return false;
  }

  return true;
}

bool ShouldSniffMimeType(const GURL& url, const std::string& mime_type) {
  // We are willing to sniff the mime type for HTTP, HTTPS, and FTP
  bool sniffable_scheme = url.is_empty() ||
                          url.SchemeIs("http") ||
                          url.SchemeIs("https") ||
                          url.SchemeIs("ftp") ||
                          url.SchemeIsFile();
  if (!sniffable_scheme) {
    return false;
  }

  static const char* kSniffableTypes[] = {
    // Many web servers are misconfigured to send text/plain for many
    // different types of content.
    "text/plain",
    // We want to sniff application/octet-stream for
    // application/x-chrome-extension, but nothing else.
    "application/octet-stream",
    // XHTML and Atom/RSS feeds are often served as plain xml instead of
    // their more specific mime types.
    "text/xml",
    "application/xml",
  };
  
  
  for (size_t i = 0; i < arraysize(kSniffableTypes); ++i) {
    if (mime_type == kSniffableTypes[i]) {
      return true;
    }
  }
  if (IsUnknownMimeType(mime_type)) {
    // The web server didn't specify a content type or specified a mime
    // type that we ignore.
    return true;
  }

  return false;
}

bool SniffMimeType(const char* content, size_t content_size,
                   const GURL& url, const std::string& type_hint,
                   std::string* result) {
  DCHECK_LT(content_size, 1000000U);  // sanity check
  DCHECK(content);
  DCHECK(result);

  // By default, we assume we have enough content.
  // Each sniff routine may unset this if it wasn't provided enough content.
  bool have_enough_content = true;

  // By default, we'll return the type hint.
  // Each sniff routine may modify this if it has a better guess..
  result->assign(type_hint);

  // Cache information about the type_hint
  const bool hint_is_unknown_mime_type = IsUnknownMimeType(type_hint);

  // First check for HTML
  if (hint_is_unknown_mime_type) {
    // We're only willing to sniff HTML if the server has not supplied a mime
    // type, or if the type it did supply indicates that it doesn't know what
    // the type should be.
    if (SniffForHTML(content, content_size, &have_enough_content, result))
      return true;  // We succeeded in sniffing HTML.  No more content needed.
  }

  // We're only willing to sniff for binary in 3 cases:
  // 1. The server has not supplied a mime type.
  // 2. The type it did supply indicates that it doesn't know what the type
  //    should be.
  // 3. The type is "text/plain" which is the default on some web servers and
  //    could be indicative of a mis-configuration that we shield the user from.
  const bool hint_is_text_plain = (type_hint == "text/plain");
  if (hint_is_unknown_mime_type || hint_is_text_plain) {
    if (!SniffBinary(content, content_size, &have_enough_content, result)) {
      // If the server said the content was text/plain and it doesn't appear
      // to be binary, then we trust it.
      if (hint_is_text_plain) {
        return have_enough_content;
      }
    }
  }

  // If we have plain XML, sniff XML subtypes.
  if (type_hint == "text/xml" || type_hint == "application/xml") {
    // We're not interested in sniffing these types for images and the like.
    // Instead, we're looking explicitly for a feed.  If we don't find one
    // we're done and return early.
    if (SniffXML(content, content_size, &have_enough_content, result))
      return true;
    return have_enough_content;
  }

  // CRX files (chrome extensions) have a special sniffing algorithm. It is
  // tighter than the others because we don't have to match legacy behavior.
  if (SniffCRX(content, content_size, url, type_hint,
               &have_enough_content, result))
    return true;

  // We're not interested in sniffing for magic numbers when the type_hint
  // is application/octet-stream.  Time to bail out.
  if (type_hint == "application/octet-stream")
    return have_enough_content;

  // Now we look in our large table of magic numbers to see if we can find
  // anything that matches the content.
  if (SniffForMagicNumbers(content, content_size,
                           &have_enough_content, result))
    return true;  // We've matched a magic number.  No more content needed.

  return have_enough_content;
}
}// namespace android

