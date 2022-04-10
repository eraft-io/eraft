#include <cassert>
#include <set>
#include "regex.h"

namespace qedis
{

bool NotGlobRegex(const char* pattern, std::size_t plen)
{
    for (std::size_t i(0); i < plen; ++ i)
    {
        if (pattern[i] == '?' ||
            pattern[i] == '\\' ||
            pattern[i] == '[' ||
            pattern[i] == ']' ||
            pattern[i] == '*' ||
            pattern[i] == '^' ||
            pattern[i] == '-')
            return false; //  may be regex, may not, who cares?
    }

    return true; // must not be regex
}

QGlobRegex::QGlobRegex(const char* pattern, std::size_t plen,
                       const char* text, std::size_t tlen)
{
    SetPattern(pattern, plen);
    SetText(text, tlen);
}

void QGlobRegex::SetPattern(const char* pattern, std::size_t plen)
{
    pattern_ = pattern;
    pLen_ = plen;
    pOff_ = 0;
}

void QGlobRegex::SetText(const char* text, std::size_t tlen)
{
    text_ = text;
    tLen_ = tlen;
    tOff_ = 0;
}

bool QGlobRegex::TryMatch()
{
    while (pOff_ < pLen_)
    {
        switch (pattern_[pOff_])
        {
        case '*':
            return  _ProcessStar();

        case '?':
            if (!_ProcessQuestion())
                return false;

            break;

        case '[':
            if (!_ProcessBracket())
                return false;

            break;

        case '\\':
            if (pOff_ + 1 < pLen_ &&
                pattern_[pOff_ + 1] == '[')
                ++ pOff_;
            // fall through;

        default:
            if (tOff_ >= tLen_)
                return false;

            if (pattern_[pOff_] != text_[tOff_])
                return false;

            ++ pOff_;
            ++ tOff_;
            break;
        }
    }
    
    return _IsMatch();
}

bool QGlobRegex::_ProcessStar()
{
    assert(pattern_[pOff_] == '*');

    do
    {
        ++ pOff_;
    } while (pOff_ < pLen_ && pattern_[pOff_] == '*');

    if (pOff_ == pLen_)
        return  true;

    while (tOff_ < tLen_)
    {
        std::size_t oldpoff = pOff_;
        std::size_t oldtoff = tOff_;
        if (TryMatch())
        {
            return true;
        }

        pOff_ = oldpoff;
        tOff_ = oldtoff;
        ++ tOff_;
    }

    return false;
}

bool QGlobRegex::_ProcessQuestion()
{
    assert(pattern_[pOff_] == '?');

    while (pOff_ < pLen_)
    {
        if (pattern_[pOff_] != '?')
            break;

        if (tOff_ == tLen_)
        {
            return false; // str is too short
        }

        ++ pOff_;
        ++ tOff_;
    }

    return true;
}

            
bool  QGlobRegex::_ProcessBracket()
{
    assert(pattern_[pOff_] == '[');

    if (pOff_ + 1 >= pLen_)
    {
        //std::cerr << "expect ] at end\n";
        return  false;
    }

    ++ pOff_;

    bool  include = true;
    if (pattern_[pOff_] == '^')
    {
        include = false;
        ++ pOff_;
    }

    std::set<char>  chars;
    
    if (pOff_ < pLen_ && pattern_[pOff_] == ']')
    {
        chars.insert(']'); // No allowed empty brackets.
        ++ pOff_;
    }

    std::set<std::pair<int, int> >  spans;
    while (pOff_ < pLen_ && pattern_[pOff_] != ']')
    {
        if ((pOff_ + 3) < pLen_ && pattern_[pOff_ + 1] == '-')
        {
            int start = pattern_[pOff_];
            int end   = pattern_[pOff_ + 2];

            if (start == end)
            {
                chars.insert(start);
            }
            else
            {
                if (start > end)
                    std::swap(start, end);

                spans.insert(std::make_pair(start, end));
            }

            pOff_ += 3;
        }
        else 
        {
            chars.insert(pattern_[pOff_]);
            ++ pOff_;
        }
    }

    if (pOff_ == pLen_)
    {
        //std::cerr << "expect ]\n";
        return  false;
    }
    else
    {
        assert (pattern_[pOff_] == ']');
        ++ pOff_;
    }

    if (chars.count(text_[tOff_]) > 0)
    {
        if (include)
        {
            ++ tOff_;
            return true;
        }
        else
        {
            return false;
        }
    }

    for (const auto& pair : spans)
    {
        if (text_[tOff_] >= pair.first && text_[tOff_] <= pair.second)
        {
            if (include)
            {
                ++ tOff_;
                return true;
            }
            else
            {
                return false;
            }
        }
    }

    if (include)
    {
        return false;
    }
    else
    {
        ++ tOff_;
        return true;
    }
}

bool QGlobRegex::_IsMatch() const
{
    return pOff_ == pLen_ && tLen_ == tOff_;
}

}