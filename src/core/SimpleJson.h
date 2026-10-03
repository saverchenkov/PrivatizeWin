#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <cctype>

namespace PrivatizeWin {

enum class JsonType {
    Null,
    Boolean,
    Number,
    String,
    Array,
    Object
};

class JsonValue {
public:
    JsonType type{ JsonType::Null };
    bool boolValue{ false };
    double numberValue{ 0.0 };
    std::string stringValue;
    std::vector<JsonValue> arrayValue;
    std::map<std::string, JsonValue> objectValue;

    JsonValue() = default;
    JsonValue(bool b) : type(JsonType::Boolean), boolValue(b) {}
    JsonValue(double n) : type(JsonType::Number), numberValue(n) {}
    JsonValue(int n) : type(JsonType::Number), numberValue(static_cast<double>(n)) {}
    JsonValue(const std::string& s) : type(JsonType::String), stringValue(s) {}
    JsonValue(const char* s) : type(JsonType::String), stringValue(s) {}
    JsonValue(JsonType t) : type(t) {}

    JsonValue& operator=(const std::string& s) { type = JsonType::String; stringValue = s; return *this; }
    JsonValue& operator=(const char* s) { type = JsonType::String; stringValue = s; return *this; }
    JsonValue& operator=(bool b) { type = JsonType::Boolean; boolValue = b; return *this; }
    JsonValue& operator=(double n) { type = JsonType::Number; numberValue = n; return *this; }
    JsonValue& operator=(int n) { type = JsonType::Number; numberValue = static_cast<double>(n); return *this; }

    bool isObject() const { return type == JsonType::Object; }
    bool isArray() const { return type == JsonType::Array; }
    bool isString() const { return type == JsonType::String; }
    bool isBool() const { return type == JsonType::Boolean; }
    bool isNumber() const { return type == JsonType::Number; }
    bool isNull() const { return type == JsonType::Null; }

    const JsonValue& operator[](const std::string& key) const {
        static JsonValue nullVal;
        if (type != JsonType::Object) return nullVal;
        auto it = objectValue.find(key);
        return (it != objectValue.end()) ? it->second : nullVal;
    }

    JsonValue& operator[](const std::string& key) {
        if (type != JsonType::Object) {
            type = JsonType::Object;
            objectValue.clear();
        }
        return objectValue[key];
    }

    bool has(const std::string& key) const {
        return type == JsonType::Object && objectValue.find(key) != objectValue.end();
    }

    std::string toString(int indent = 0) const {
        std::ostringstream ss;
        serialize(ss, indent, 0);
        return ss.str();
    }

    static JsonValue parse(const std::string& str) {
        size_t idx = 0;
        skipWhitespace(str, idx);
        if (idx >= str.size()) {
            throw std::runtime_error("Empty JSON input");
        }
        JsonValue val = parseValue(str, idx);
        skipWhitespace(str, idx);
        if (idx < str.size()) {
            throw std::runtime_error("Unexpected trailing characters in JSON input");
        }
        return val;
    }

private:
    static void skipWhitespace(const std::string& s, size_t& idx) {
        while (idx < s.size() && (s[idx] == ' ' || s[idx] == '\t' || s[idx] == '\r' || s[idx] == '\n')) {
            idx++;
        }
    }

    static JsonValue parseValue(const std::string& s, size_t& idx, int depth = 0) {
        if (depth > 64) {
            throw std::runtime_error("JSON depth limit exceeded");
        }
        skipWhitespace(s, idx);
        if (idx >= s.size()) {
            throw std::runtime_error("Unexpected end of JSON input");
        }

        char c = s[idx];
        if (c == '{') return parseObject(s, idx, depth + 1);
        if (c == '[') return parseArray(s, idx, depth + 1);
        if (c == '"') return parseString(s, idx);
        if (c == 't' || c == 'f') return parseBool(s, idx);
        if (c == 'n') return parseNull(s, idx);
        if (c == '-' || std::isdigit(static_cast<unsigned char>(c))) return parseNumber(s, idx);

        throw std::runtime_error(std::string("Invalid JSON token starting with '") + c + "'");
    }

    static JsonValue parseObject(const std::string& s, size_t& idx, int depth) {
        JsonValue val(JsonType::Object);
        idx++; // skip '{'
        skipWhitespace(s, idx);
        if (idx < s.size() && s[idx] == '}') {
            idx++;
            return val;
        }

        while (idx < s.size()) {
            skipWhitespace(s, idx);
            if (idx >= s.size() || s[idx] != '"') {
                throw std::runtime_error("Expected string key in JSON object");
            }
            JsonValue keyVal = parseString(s, idx);
            std::string key = keyVal.stringValue;

            skipWhitespace(s, idx);
            if (idx >= s.size() || s[idx] != ':') {
                throw std::runtime_error("Expected ':' after key in JSON object");
            }
            idx++; // skip ':'

            val.objectValue[key] = parseValue(s, idx, depth);

            skipWhitespace(s, idx);
            if (idx < s.size() && s[idx] == ',') {
                idx++;
                skipWhitespace(s, idx);
                if (idx < s.size() && s[idx] == '}') {
                    throw std::runtime_error("Trailing comma in JSON object");
                }
                continue;
            }
            if (idx < s.size() && s[idx] == '}') {
                idx++;
                return val;
            }
            throw std::runtime_error("Expected ',' or '}' in JSON object");
        }
        throw std::runtime_error("Unterminated JSON object");
    }

    static JsonValue parseArray(const std::string& s, size_t& idx, int depth) {
        JsonValue val(JsonType::Array);
        idx++; // skip '['
        skipWhitespace(s, idx);
        if (idx < s.size() && s[idx] == ']') {
            idx++;
            return val;
        }

        while (idx < s.size()) {
            val.arrayValue.push_back(parseValue(s, idx, depth));
            skipWhitespace(s, idx);
            if (idx < s.size() && s[idx] == ',') {
                idx++;
                skipWhitespace(s, idx);
                if (idx < s.size() && s[idx] == ']') {
                    throw std::runtime_error("Trailing comma in JSON array");
                }
                continue;
            }
            if (idx < s.size() && s[idx] == ']') {
                idx++;
                return val;
            }
            throw std::runtime_error("Expected ',' or ']' in JSON array");
        }
        throw std::runtime_error("Unterminated JSON array");
    }

    static JsonValue parseString(const std::string& s, size_t& idx) {
        idx++; // skip opening '"'
        std::string res;
        while (idx < s.size()) {
            char c = s[idx++];
            if (c == '"') return JsonValue(res);
            if (static_cast<unsigned char>(c) < 0x20) {
                throw std::runtime_error("Unescaped control character in string");
            }
            if (c == '\\') {
                if (idx >= s.size()) throw std::runtime_error("Unterminated escape sequence in string");
                char esc = s[idx++];
                if (esc == '"') res += '"';
                else if (esc == '\\') res += '\\';
                else if (esc == '/') res += '/';
                else if (esc == 'b') res += '\b';
                else if (esc == 'f') res += '\f';
                else if (esc == 'n') res += '\n';
                else if (esc == 'r') res += '\r';
                else if (esc == 't') res += '\t';
                else if (esc == 'u') {
                    if (idx + 4 > s.size()) throw std::runtime_error("Incomplete \\uXXXX escape sequence");
                    std::string hexStr = s.substr(idx, 4);
                    for (char hc : hexStr) {
                        if (!std::isxdigit(static_cast<unsigned char>(hc))) {
                            throw std::runtime_error("Invalid hex digit in \\uXXXX escape sequence");
                        }
                    }
                    idx += 4;
                    unsigned int codePoint = static_cast<unsigned int>(std::stoul(hexStr, nullptr, 16));
                    if (codePoint <= 0x7F) {
                        res += static_cast<char>(codePoint);
                    } else if (codePoint <= 0x7FF) {
                        res += static_cast<char>(0xC0 | ((codePoint >> 6) & 0x1F));
                        res += static_cast<char>(0x80 | (codePoint & 0x3F));
                    } else {
                        res += static_cast<char>(0xE0 | ((codePoint >> 12) & 0x0F));
                        res += static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F));
                        res += static_cast<char>(0x80 | (codePoint & 0x3F));
                    }
                } else {
                    throw std::runtime_error(std::string("Invalid escape sequence: \\") + esc);
                }
            } else {
                res += c;
            }
        }
        throw std::runtime_error("Unterminated string literal");
    }

    static JsonValue parseNumber(const std::string& s, size_t& idx) {
        size_t start = idx;
        if (idx < s.size() && s[idx] == '-') {
            idx++;
        }
        if (idx >= s.size() || !std::isdigit(static_cast<unsigned char>(s[idx]))) {
            throw std::runtime_error("Expected digit in number");
        }

        if (s[idx] == '0') {
            idx++;
            if (idx < s.size() && std::isdigit(static_cast<unsigned char>(s[idx]))) {
                throw std::runtime_error("Leading zeros are not permitted in numbers");
            }
        } else {
            while (idx < s.size() && std::isdigit(static_cast<unsigned char>(s[idx]))) {
                idx++;
            }
        }

        // Optional fraction
        if (idx < s.size() && s[idx] == '.') {
            idx++;
            if (idx >= s.size() || !std::isdigit(static_cast<unsigned char>(s[idx]))) {
                throw std::runtime_error("Expected digit after decimal point");
            }
            while (idx < s.size() && std::isdigit(static_cast<unsigned char>(s[idx]))) {
                idx++;
            }
        }

        // Optional exponent
        if (idx < s.size() && (s[idx] == 'e' || s[idx] == 'E')) {
            idx++;
            if (idx < s.size() && (s[idx] == '+' || s[idx] == '-')) {
                idx++;
            }
            if (idx >= s.size() || !std::isdigit(static_cast<unsigned char>(s[idx]))) {
                throw std::runtime_error("Expected digit in exponent");
            }
            while (idx < s.size() && std::isdigit(static_cast<unsigned char>(s[idx]))) {
                idx++;
            }
        }

        const std::string numStr = s.substr(start, idx - start);
        size_t processed = 0;
        double num = 0.0;
        try {
            num = std::stod(numStr, &processed);
            if (processed != numStr.size()) {
                throw std::runtime_error("Trailing characters in number");
            }
        } catch (...) {
            throw std::runtime_error("Invalid number format in JSON");
        }
        return JsonValue(num);
    }

    static JsonValue parseBool(const std::string& s, size_t& idx) {
        if (s.compare(idx, 4, "true") == 0) {
            idx += 4;
            return JsonValue(true);
        }
        if (s.compare(idx, 5, "false") == 0) {
            idx += 5;
            return JsonValue(false);
        }
        throw std::runtime_error("Invalid boolean value in JSON");
    }

    static JsonValue parseNull(const std::string& s, size_t& idx) {
        if (s.compare(idx, 4, "null") == 0) {
            idx += 4;
            return JsonValue(JsonType::Null);
        }
        throw std::runtime_error("Invalid null literal in JSON");
    }

    static void escapeAndWrite(std::ostringstream& ss, const std::string& str) {
        ss << '"';
        for (char c : str) {
            if (c == '"') ss << "\\\"";
            else if (c == '\\') ss << "\\\\";
            else if (c == '\n') ss << "\\n";
            else if (c == '\r') ss << "\\r";
            else if (c == '\t') ss << "\\t";
            else ss << c;
        }
        ss << '"';
    }

    void serialize(std::ostringstream& ss, int indent, int level) const {
        std::string pad(level * indent, ' ');
        std::string nextPad((level + 1) * indent, ' ');

        switch (type) {
        case JsonType::Null: ss << "null"; break;
        case JsonType::Boolean: ss << (boolValue ? "true" : "false"); break;
        case JsonType::Number:
            if (numberValue == static_cast<int64_t>(numberValue)) {
                ss << static_cast<int64_t>(numberValue);
            } else {
                ss << numberValue;
            }
            break;
        case JsonType::String: {
            escapeAndWrite(ss, stringValue);
            break;
        }
        case JsonType::Array: {
            if (arrayValue.empty()) {
                ss << "[]";
                return;
            }
            ss << '[';
            if (indent > 0) ss << '\n';
            for (size_t i = 0; i < arrayValue.size(); ++i) {
                if (indent > 0) ss << nextPad;
                arrayValue[i].serialize(ss, indent, level + 1);
                if (i + 1 < arrayValue.size()) ss << ',';
                if (indent > 0) ss << '\n';
            }
            if (indent > 0) ss << pad;
            ss << ']';
            break;
        }
        case JsonType::Object: {
            if (objectValue.empty()) {
                ss << "{}";
                return;
            }
            ss << '{';
            if (indent > 0) ss << '\n';
            size_t count = 0;
            for (const auto& [k, v] : objectValue) {
                if (indent > 0) ss << nextPad;
                escapeAndWrite(ss, k);
                ss << ": ";
                v.serialize(ss, indent, level + 1);
                if (++count < objectValue.size()) ss << ',';
                if (indent > 0) ss << '\n';
            }
            if (indent > 0) ss << pad;
            ss << '}';
            break;
        }
        }
    }
};

} // namespace PrivatizeWin
