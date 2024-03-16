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
