#ifndef RESPONSEEXTRACTORS_H
#define RESPONSEEXTRACTORS_H
#include <string>
#include <limits.h>
#include <map>
#include <stdint.h>
#include <vector>

#define NUMBER_NOT_FOUND INT_MIN

const std::map<uint16_t, std::string> CME_ERRORS = {
    {0, "Phone failure"}, {1, "No connection to phone"}, {2, "Phone-adaptor link reserved"},
    {3, "Operation not allowed"}, {4, "Operation not supported"}, {5, "PH-SIM PIN required"},
    {6, "PH-FSIM PIN required"}, {7, "PH-FSIM PUK required"}, {10, "SIM not inserted"},
    {11, "SIM PIN required"}, {12, "SIM PUK required"}, {13, "SIM failure"}, {14, "SIM busy"},
    {15, "SIM wrong"}, {16, "Incorrect password"}, {17, "SIM PIN2 required"}, {18, "SIM PUK2 required"},
    {20, "Memory full"}, {21, "Invalid index"}, {22, "Not found"}, {23, "Memory failure"},
    {24, "Text string too long"}, {25, "Invalid characters in text string"},
    {26, "Dial string too long"}, {27, "Invalid characters in dial string"},
    {30, "No network service"}, {31, "Network timeout"}, {32, "Network not allowed - emergency calls only"},
    {40, "Network personalization PIN required"}, {41, "Network personalization PUK required"},
    {42, "Network subset personalization PIN required"}, {43, "Network subset personalization PUK required"},
    {44, "Service provider personalization PIN required"}, {45, "Service provider personalization PUK required"},
    {46, "Corporate personalization PIN required"}, {47, "Corporate personalization PUK required"},
    {901, "Audio unknown error"}, {902, "Audio invalid parameters"}, {903, "Audio operation not supported"},
    {904, "Audio device busy"}
};

const std::map<uint16_t, std::string> CMS_ERRORS = {
    {107, "Other General problems"}, {300, "ME failure"}, {301, "SMS ME reserved"},
    {302, "Operation not allowed"}, {303, "Operation not supported"},
    {304, "Invalid PDU mode"}, {305, "Invalid text mode"}, {310, "SIM not inserted"},
    {311, "SIM PIN necessary"}, {312, "PH-SIM PIN necessary"}, {313, "SIM failure"},
    {314, "SIM busy"}, {315, "SIM wrong"}, {316, "SIM PUK required"},
    {317, "SIM PIN2 required"}, {318, "SIM PUK2 required"}, {320, "Memory failure"},
    {321, "Invalid memory index"}, {322, "Memory full"}, {330, "SMSC address unknown"},
    {331, "No network"}, {332, "Network timeout"}, {340, "No +CNMA acknowledgement expected"},
    {350, "Unknown"}, {500, "Unknown"}, {510, "Message blocked"}
};

int extractNumericEnum(const std::string& s);
bool isResponseSuccess(const std::string& s);
std::string extractNumericEnumAsString(const std::string& s);
std::string flattenString(std::string s);
std::string replaceSubstring(std::string s, const std::string& sub1, const std::string& sub2);
bool isError(const std::string& s);
std::string getErrorMessage(const std::string& s);
std::string extractSimpleState(const std::string& s);
std::vector<std::string> splitString(std::string s, std::string delim);
std::vector<std::string> flattenAndSplitString(std::string s, std::string delim);
std::string quoteString(std::string& s);

#endif // RESPONSEEXTRACTORS_H
