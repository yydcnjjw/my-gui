#pragma once

#include <boost/locale/encoding.hpp>
#include <boost/locale/generator.hpp>
#include <locale>
#include <codecvt>

namespace codecvt = boost::locale::conv;
typedef std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t>
    utf16_codecvt;
