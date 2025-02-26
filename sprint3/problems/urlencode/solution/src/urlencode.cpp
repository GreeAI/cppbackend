#include "urlencode.h"
#include <sstream>

std::string FromCharToHex(unsigned char c){
    std::ostringstream result;
    result << '%' << std::hex << int(c);
    return result.str();
}

std::string UrlEncode(std::string_view str) {
    std::string result;
    for(unsigned char c : str){
        if(RESERVED_CHARS.count(c) || c < 32 || c >= 128){
            std::string encoded_char = FromCharToHex(c);
            result.append(encoded_char);
        } else {
            if(c == ' '){
                result.push_back('+');
            } else {
               result.push_back(c); 
            }
            
        }
    }

    return result;
}
