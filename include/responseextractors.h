#ifndef RESPONSEEXTRACTORS_H
#define RESPONSEEXTRACTORS_H
#include <string>
#include <limits.h>

#define NUMBER_NOT_FOUND INT_MIN

int extractNumericEnum(std::string s);
bool isResponseSuccess(std::string s);
std::string extractNumericEnumAsString(std::string s);


#endif // RESPONSEEXTRACTORS_H
