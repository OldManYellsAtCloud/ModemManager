#include <gtest/gtest.h>
#include "responseextractors.h"

TEST(Misc_Suite, SplitString){
    std::string s = "abc def ghi jkl";
    std::vector<std::string> res = splitString(s, " ");

    EXPECT_EQ(res.at(0), "abc");
    EXPECT_EQ(res.at(1), "def");
    EXPECT_EQ(res.at(2), "ghi");
    EXPECT_EQ(res.at(3), "jkl");
}

TEST(Misc_Suite, SplitString_MultiCharDelim){
    std::string s = "abc+=def+=ghi+=jkl";
    std::vector<std::string> res = splitString(s, "+=");
    EXPECT_EQ(res.at(0), "abc");
    EXPECT_EQ(res.at(1), "def");
    EXPECT_EQ(res.at(2), "ghi");
    EXPECT_EQ(res.at(3), "jkl");
}

TEST(Misc_Suite, FlattenString_rn){
    std::string s = "abc\r\ndef\r\nghi\r\njkl";
    std::string res = flattenString(s);
    EXPECT_EQ(res, "abc def ghi jkl");
}

TEST(Misc_Suite, FlattenString_r){
    std::string s = "abc\rdef\rghi\rjkl";
    std::string res = flattenString(s);
    EXPECT_EQ(res, "abc def ghi jkl");
}

TEST(Misc_Suite, ReplaceSubString){
    std::string s = "abc def ghi jkl";
    std::string toReplace = "f g";
    std::string replaceWith = "lop";
    std::string res = replaceSubstring(s, toReplace, replaceWith);
    EXPECT_EQ(res, "abc delophi jkl");
}

TEST(Misc_Suite, ReplaceSubString_noSubstringToReplace){
    std::string s = "abc def ghi jkl";
    std::string toReplace = "xxx";
    std::string replaceWith = "lop";
    std::string res = replaceSubstring(s, toReplace, replaceWith);
    EXPECT_EQ(res, "abc def ghi jkl");
}

TEST(Misc_Suite, ReplaceSubString_multipleInstances){
    std::string s = "abc abc abc abc";
    std::string toReplace = "abc";
    std::string replaceWith = "def";
    std::string res = replaceSubstring(s, toReplace, replaceWith);
    EXPECT_EQ(res, "def def def def");
}

TEST(Misc_Suite, QuoteString_full){
    std::string s = "text";
    quoteString(s);
    EXPECT_EQ(s, "\"text\"");
}

TEST(Misc_Suite, QuoteString_left){
    std::string s = "text\"";
    quoteString(s);
    EXPECT_EQ(s, "\"text\"");
}

TEST(Misc_Suite, QuoteString_right){
    std::string s = "\"text";
    quoteString(s);
    EXPECT_EQ(s, "\"text\"");
}

TEST(Misc_Suite, QuoteString_multiword){
    std::string s = "one two \" three four";
    quoteString(s);
    EXPECT_EQ(s, "\"one two \" three four\"");
}

TEST(Misc_Suite, SubstringInVector){
    std::vector<std::string> v {"abc", "bcd", "cde"};
    std::string s = findSubstringInVector(v, "bc");
    EXPECT_EQ(s, "abc");
}

TEST(Misc_Suite, SubstringInVector_empty_vector){
    std::vector<std::string> v;
    std::string s = findSubstringInVector(v, "bc");
    EXPECT_EQ(s, "");
}

TEST(Misc_Suite, SubstringInVector_not_found){
    std::vector<std::string> v {"abc", "bcd", "cde"};
    std::string s = findSubstringInVector(v, "xx");
    EXPECT_EQ(s, "");
}
