#include "htmldecode.h"
#include <regex>

std::string HtmlDecode(std::string_view str) {
    std::regex lt("&(lt|LT);?");
    std::regex gt("&(gt|GT);?");
    std::regex amp("&(amp|AMP);?");
    std::regex apos("&(apos|APOS);?");
    std::regex qout("&(quot|QUOT);?");

    std::string ret(str);

    ret = std::regex_replace(ret, lt, "<");
    ret = std::regex_replace(ret, gt, ">");
    ret = std::regex_replace(ret, amp, "&");

    ret = std::regex_replace(ret, apos, "\'");
    ret = std::regex_replace(ret, qout, "\"");

    return ret;
}
