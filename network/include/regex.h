#pragma once

#include <string.h>
#include <string>

namespace qedis
{

bool NotGlobRegex(const char* pattern, std::size_t plen);

class QGlobRegex
{
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

    const char*     pattern_;
    std::size_t     pLen_;
    std::size_t     pOff_;

    const char*     text_;
    std::size_t     tLen_;
    std::size_t     tOff_;
};

inline bool glob_match(const char* pattern, std::size_t plen,
                       const char* text, std::size_t tlen)
{
    QGlobRegex   rgx;
    rgx.SetPattern(pattern, plen);
    rgx.SetText(text, tlen);

    return rgx.TryMatch();
}

inline bool glob_match(const char* pattern,
                       const char* text)
{
    return glob_match(pattern, strlen(pattern), text, strlen(text));
}

inline bool glob_match(const std::string& pattern,
                       const std::string& text)
{
    return glob_match(pattern.c_str(), pattern.size(),
                      text.c_str(), text.size());
}

inline bool glob_search(const char* pattern,
                       const char* str)
{
    std::string sPattern("*");
    sPattern += pattern;
    sPattern += "*";

    QGlobRegex   rgx;
    rgx.SetPattern(sPattern.c_str(), sPattern.size());
    rgx.SetText(str, strlen(str));

    return rgx.TryMatch();
}
    
}
