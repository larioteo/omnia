#pragma once

#include "Omnia/Omnia.pch"

// String Extensions
namespace Omnia {

constexpr inline size_t operator"" _hash(const char *value, size_t count) {
    return value == "" ? 0 : std::hash<string>{}(value);
}

class String {
    String() = delete;
    ~String() = delete;

public:
    static bool Contains(string_view value, string_view token, bool caseSensitive = false) noexcept {
        if (value.length() < token.length()) return false;
        if (caseSensitive) {
            return value.find(token) != string::npos;
        } else {
            auto it = std::search(value.begin(), value.end(), token.begin(), token.end(), [](char char1, char char2) {
                return std::toupper(char1) == std::toupper(char2);
            });
            return (it != value.end());
        }
    }
    static bool EndsWith(string_view value, string_view token, bool caseSensitive = false) noexcept {
        if (value.length() < token.length()) return false;
        if (caseSensitive) {
            return value.compare(value.length() - token.length(), token.length(), token) == 0;
        } else {
            auto it = std::search(value.begin(), value.end(), token.begin(), token.end(), [](char char1, char char2) {
                return std::toupper(char1) == std::toupper(char2);
            });
            return (it != value.end());
        }
    }
    static bool StartsWith(string_view value, string_view token, bool caseSensitive = false) noexcept {
        if (value.length() < token.length()) return false;
        if (caseSensitive) {
            return value.compare(0, token.length(), token) == 0;
        } else {
            auto it = std::search(value.begin(), value.end(), token.begin(), token.end(), [](char char1, char char2) {
                return std::toupper(char1) == std::toupper(char2);
            });
            return (it != value.end());
        }
    }

    static bool IsNumeric(string_view value) noexcept {
        return std::all_of(value.cbegin(), value.cend(), [](auto c) {
            return (c >= '0' && c <= '9') || c == '-' || c == '.';
        });
    }
    static bool IsDecimal(string_view value) noexcept {
        return std::all_of(value.cbegin(), value.cend(), [](auto c) {
            return (c >= '0' && c <= '9');
        });
    }
    static bool IsHexadecimal(string_view value) noexcept {
        return std::all_of(value.cbegin(), value.cend(), [](auto c) {
            return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
        });
    }
    static bool IsOctal(string_view value) noexcept {
        return std::all_of(value.cbegin(), value.cend(), [](auto c) {
            return (c >= '0' && c <= '7');
        });
    }

    static void Replace(string &value, string_view token, string_view to) noexcept {
        auto position = value.find(token);
        while (position != std::string::npos) {
            value.replace(position, token.length(), to);
            position = value.find(token, position + token.size());
        }
    }
    static vector<string> Split(const string &value, char seperator) noexcept {
        stringstream stream(value);
        string token;
        vector<string> tokens;

        while (std::getline(stream, token, seperator)) {
            tokens.emplace_back(token);
        }
        return tokens;
    }
    static void ToLower(string &value) noexcept {
        std::transform(value.begin(), value.end(), value.begin(), ::tolower);
    }
    static void ToLower(std::wstring &value) noexcept {
        std::transform(value.begin(), value.end(), value.begin(), ::tolower);
    }
    static void ToUpper(string &value) noexcept {
        std::transform(value.begin(), value.end(), value.begin(), ::toupper);
    }
    static void ToUpper(std::wstring &value) noexcept {
        std::transform(value.begin(), value.end(), value.begin(), ::toupper);
    }

    static void Test() {
        auto string0 = "b"_hash;

        string string1 = "First";
        string string2 = "Second";
        string string3 = "third";
        string string4 = "Fourth";
        string string5 = "First Second Third";
        string string6 = "123.456-789";
        string string7 = "a123.456-789";
        string string71 = "0123456789";
        string string72 = "0123456789ABCDEF";
        string string73 = "01234567";
        string string8 = "This is a very long sentance for a small string!";
        string string9 = "What the ... should i do. Maybe it would be my 1st try to 3489032840921384092384902318490823149203945803294.";
        std::wstring wstring1 = L"Dies ist ein sehr langer Satz für einen String, deshalb sollter er mit bedacht eingesetzt werden!";
        std::wstring wstring2 = L"Ein paar Zeichen zum testen #+äü+ö€";

        auto result00 = String::Contains(string5, string2);
        auto result01 = String::Contains(string5, string4);

        auto result10 = String::EndsWith(string5, string3);
        auto result11 = String::EndsWith(string5, string1);
        
        auto result20 = String::StartsWith(string5, string1);
        auto result21 = String::StartsWith(string5, string3);
        
        auto result30 = String::IsNumeric(string6);
        auto result31 = String::IsNumeric(string7);

        auto result40 = String::IsDecimal(string71);
        auto result41 = String::IsHexadecimal(string72);
        auto result42 = String::IsOctal(string73);

        String::Replace(string5, "second", "sec");
        String::Replace(string7, "a", "b");

        auto result50 = String::Split(string5, ' ');
        auto result51 = String::Split(string7, '-');

        String::ToLower(string8);
        String::ToUpper(string8);
        String::ToLower(string9);
        String::ToUpper(string9);

        String::ToLower(wstring1);
        String::ToUpper(wstring1);
        String::ToLower(wstring2);
        String::ToUpper(wstring2);

        AppLog(string0);
    }
};

}
