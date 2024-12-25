#include "responseextractors.h"

#define NUMBERS "0123456789"


/**
 * @brief extractNumericEnum
 * Extract a numeric value from a response, which is in the form of
 * "xxx: 123"
 * @param s
 * @return
 */
int extractNumericEnum(const std::string& s)
{
    std::string number = extractNumericEnumAsString(s);
    if (!number.length())
        return NUMBER_NOT_FOUND;
    return std::stoi(number);
}

std::string extractNumericEnumAsString(const std::string& s){
    size_t start = s.find_first_of(NUMBERS);

    if (start == std::string::npos)
        return "";

    size_t end = s.find_first_not_of(NUMBERS, start + 1);
    std::string number = s.substr(start, end - start);
    return number;
}

bool isResponseSuccess(const std::string& s)
{
    return s.find("OK") != std::string::npos;
}

std::string flattenString(std::string s)
{
    s = replaceSubstring(s, "\n", "");
    s = replaceSubstring(s, "\r", " ");
    return s;
}

std::string replaceSubstring(std::string s, const std::string& sub1, const std::string& sub2)
{
    size_t idx;
    while ((idx = s.find(sub1)) != std::string::npos){
        s.replace(idx, sub1.size(), sub2);
    }
    return s;
}

bool isError(const std::string& s)
{
    return s.find("ERROR:") != std::string::npos;
}

std::string getErrorMessage(const std::string& s)
{
    int errorCode = extractNumericEnum(s);
    std::string ret;
    if (s.find("ERROR") != std::string::npos && errorCode == NUMBER_NOT_FOUND){
        ret = "Generic error";
    } else if (s.find("CME ERROR") != std::string::npos && CME_ERRORS.contains(errorCode)) {
        ret = CME_ERRORS.at(errorCode);
    } else if (s.find("CMS ERROR") != std::string::npos && CMS_ERRORS.contains(errorCode)) {
        ret = CMS_ERRORS.at(errorCode);
    } else {
        ret = "Unknown error: " + s;
    }
    return ret;
}

std::string extractSimpleState(const std::string &s)
{
    size_t start = s.find(":") + 2; // colon + space
    size_t end = s.find_first_of("\r\n", start); // get everything till the end of the line
    return s.substr(start, end - start);
}

std::vector<std::string> splitString(std::string s, std::string delim)
{
    std::vector<std::string> ret;
    size_t idxStart = 0;
    size_t idxEnd = s.find(delim);

    while (idxEnd != std::string::npos){
        ret.push_back(s.substr(idxStart, idxEnd - idxStart));
        idxStart = idxEnd + delim.length();
        idxEnd = s.find(delim, idxStart);
    }

    ret.push_back(s.substr(idxStart));
    return ret;
}

std::vector<std::string> flattenAndSplitString(std::string s, std::string delim)
{
    s = flattenString(s);
    return splitString(s, delim);
}

void quoteString(std::string &s)
{
    if (s[0] != '"')
        s.insert(s.begin(), '"');

    if (s[s.length() - 1] != '"')
        s.append(1, '"');
}

std::string findSubstringInVector(const std::vector<std::string> &haystack, const std::string needle)
{
    for (const std::string &s: haystack){
        if (s.find(needle) != std::string::npos) return s;
    }
    return "";
}
