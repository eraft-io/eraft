// MIT License

// Copyright (c) 2021 eraft dev group

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef ERAFT_REGEX_H_
#define ERAFT_REGEX_H_

#include <string.h>

#include <string>

namespace qedis {

bool NotGlobRegex(const char* pattern, std::size_t plen);

//
//
//
//

class QGlobRegex {
 public:
  explicit QGlobRegex(const char* pattern = 0, std::size_t plen = 0,
                      const char* text = 0, std::size_t tlen = 0);

  void SetPattern(const char* pattern, std::size_t plen);
  void SetText(const char* text, std::size_t tlen);
  bool TryMatch();

 private:
  bool _ProcessStar();
  bool _ProcessQuestion();
  bool _ProcessBracket();
  bool _IsMatch() const;

  const char* pattern_;
  std::size_t pLen_;
  std::size_t pOff_;

  const char* text_;
  std::size_t tLen_;
  std::size_t tOff_;
};

inline bool glob_match(const char* pattern, std::size_t plen, const char* text,
                       std::size_t tlen) {
  QGlobRegex rgx;
  rgx.SetPattern(pattern, plen);
  rgx.SetText(text, tlen);

  return rgx.TryMatch();
}

inline bool glob_match(const char* pattern, const char* text) {
  return glob_match(pattern, strlen(pattern), text, strlen(text));
}

inline bool glob_match(const std::string& pattern, const std::string& text) {
  return glob_match(pattern.c_str(), pattern.size(), text.c_str(), text.size());
}

inline bool glob_search(const char* pattern, const char* str) {
  std::string sPattern("*");
  sPattern += pattern;
  sPattern += "*";

  QGlobRegex rgx;
  rgx.SetPattern(sPattern.c_str(), sPattern.size());
  rgx.SetText(str, strlen(str));

  return rgx.TryMatch();
}

}  // namespace qedis

#endif