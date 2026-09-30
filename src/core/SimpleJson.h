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
        return parseValue(str, idx);
    }

private:
    static void skipWhitespace(const std::string& s, size_t& idx) {
        while (idx < s.size() && (s[idx] == ' ' || s[idx] == '\t' || s[idx] == '\r' || s[idx] == '\n')) {
            idx++;
        }
    }

    static JsonValue parseValue(const std::string& s, size_t& idx) {
        skipWhitespace(s, idx);
        if (idx >= s.size()) return JsonValue();

        char c = s[idx];
        if (c == '{') return parseObject(s, idx);
        if (c == '[') return parseArray(s, idx);
        if (c == '"') return parseString(s, idx);
        if (c == 't' || c == 'f') return parseBool(s, idx);
        if (c == 'n') return parseNull(s, idx);
        if (c == '-' || std::isdigit(static_cast<unsigned char>(c))) return parseNumber(s, idx);

        return JsonValue();
    }

    static JsonValue parseObject(const std::string& s, size_t& idx) {
        JsonValue val(JsonType::Object);
        idx++; // skip '{'
        skipWhitespace(s, idx);
        if (idx < s.size() && s[idx] == '}') {
            idx++;
            return val;
        }

        while (idx < s.size()) {
            skipWhitespace(s, idx);
            if (idx >= s.size() || s[idx] != '"') break;
            JsonValue keyVal = parseString(s, idx);
            std::string key = keyVal.stringValue;

            skipWhitespace(s, idx);
            if (idx >= s.size() || s[idx] != ':') break;
            idx++; // skip ':'

            val.objectValue[key] = parseValue(s, idx);

            skipWhitespace(s, idx);
            if (idx < s.size() && s[idx] == ',') {
                idx++;
                continue;
            }
            if (idx < s.size() && s[idx] == '}') {
                idx++;
                break;
            }
            break;
        }
        return val;
    }

    static JsonValue parseArray(const std::string& s, size_t& idx) {
        JsonValue val(JsonType::Array);
        idx++; // skip '['
        skipWhitespace(s, idx);
        if (idx < s.size() && s[idx] == ']') {
            idx++;
            return val;
        }

        while (idx < s.size()) {
            val.arrayValue.push_back(parseValue(s, idx));
            skipWhitespace(s, idx);
            if (idx < s.size() && s[idx] == ',') {
                idx++;
                continue;
            }
            if (idx < s.size() && s[idx] == ']') {
                idx++;
                break;
            }
            break;
        }
        return val;
    }

    static JsonValue parseString(const std::string& s, size_t& idx) {
        idx++; // skip '"'
        std::string res;
        while (idx < s.size()) {
            char c = s[idx++];
            if (c == '"') break;
            if (c == '\\' && idx < s.size()) {
                char esc = s[idx++];
                if (esc == '"') res += '"';
                else if (esc == '\\') res += '\\';
                else if (esc == '/') res += '/';
                else if (esc == 'b') res += '\b';
                else if (esc == 'f') res += '\f';
                else if (esc == 'n') res += '\n';
                else if (esc == 'r') res += '\r';
                else if (esc == 't') res += '\t';
                else res += esc;
            } else {
                res += c;
            }
        }
        return JsonValue(res);
    }

    static JsonValue parseNumber(const std::string& s, size_t& idx) {
        size_t start = idx;
        if (s[idx] == '-') idx++;
        while (idx < s.size() && (std::isdigit(static_cast<unsigned char>(s[idx])) || s[idx] == '.' || s[idx] == 'e' || s[idx] == 'E' || s[idx] == '+' || s[idx] == '-')) {
            idx++;
        }
        double num = 0.0;
        try {
            num = std::stod(s.substr(start, idx - start));
        } catch (...) {}
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
        return JsonValue(false);
    }

    static JsonValue parseNull(const std::string& s, size_t& idx) {
        if (s.compare(idx, 4, "null") == 0) {
            idx += 4;
        }
        return JsonValue(JsonType::Null);
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
            ss << '"';
            for (char c : stringValue) {
                if (c == '"') ss << "\\\"";
                else if (c == '\\') ss << "\\\\";
                else if (c == '\n') ss << "\\n";
                else if (c == '\r') ss << "\\r";
                else if (c == '\t') ss << "\\t";
                else ss << c;
            }
            ss << '"';
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
                ss << '"' << k << "\": ";
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
