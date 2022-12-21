#pragma once

#include <string>
#include <vector>
#include <sstream>
#include <stdlib.h>

static std::string Trim(const std::string& s) {
  size_t i = 0;
  while ((i < s.length()) && (s[i] <= 32)) {
    ++i;
  }
  size_t j = s.length();
  while ((j > 0) && (s[j - 1] <= 32)) {
    --j;
  }
  return s.substr(i, j - i);
}

static std::vector<std::string> StringSplit(const std::string& s,
                                            const std::string& d) {
  std::vector<std::string> values;
  auto remainder = Trim(s);
  const auto delimiterLength = d.length();
  while (!remainder.empty()) {
    auto delimiter = remainder.find(d);
    if (delimiter == std::string::npos) {
      values.push_back(remainder);
      remainder.clear();
    } else {
      values.push_back(Trim(remainder.substr(0, delimiter)));
      remainder = Trim(remainder.substr(delimiter + delimiterLength));
    }
  }
  return values;
}

static std::string StringsJoin(const std::vector<std::string>& v,
                               const std::string& d) {
  std::ostringstream builder;
  bool first = true;
  for (const auto& piece : v) {
    if (first) {
      first = false;
    } else {
      builder << d;
    }
    builder << piece;
  }
  return builder.str();
}
