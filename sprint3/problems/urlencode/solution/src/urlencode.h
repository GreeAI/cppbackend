#pragma once

#include <string>
#include <set>
const static std::set<unsigned> RESERVED_CHARS = {'!','#','$','&',
                                            '\'','(',')','*',
                                            '+',',','/',':',
                                            ';','=','?','@',
                                            '[',']'};


std::string FromCharToHex(unsigned char c);
/*
 * URL-кодирует строку str.
 * Пробел заменяется на +,
 * Символы, отличные от букв английского алфавита, цифр и -._~ а также зарезервированные символы
 * заменяются на их %-кодированные последовательности.
 * Зарезервированные символы: !#$&'()*+,/:;=?@[]
 */
std::string UrlEncode(std::string_view str);
