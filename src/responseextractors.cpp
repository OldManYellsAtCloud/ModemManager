#include "responseextractors.h"

#define NUMBERS "0123456789"


/**
 * @brief extractNumericEnum
 * Extract a numeric value from a response, which is in the form of
 * "xxx: 123"
 * @param s
 * @return
 */
int extractNumericEnum(std::string s)
{
    std::string number = extractNumericEnumAsString(s);
    if (!number.length())
        return NUMBER_NOT_FOUND;
    return std::stoi(number);
}

std::string extractNumericEnumAsString(std::string s){
    size_t start = s.find_first_of(NUMBERS);
    size_t end = s.find_first_not_of(NUMBERS, start + 1);
    std::string number = s.substr(start, end - start);
    return number;
}

bool isResponseSuccess(std::string s)
{
    return s.find("OK") != std::string::npos;
}

std::string flattenString(std::string s)
{
    s = replaceSubstring(s, "\n", "");
    s = replaceSubstring(s, "\r", " ");
    return s;
}

std::string replaceSubstring(std::string s, std::string sub1, std::string sub2)
{
    size_t idx;
    while ((idx = s.find(sub1)) != std::string::npos){
        s.replace(idx, sub1.size(), sub2);
    }
    return s;
}
