#include "../TestFramework.h"
#include "../../src/core/SimpleJson.h"

using namespace PrivatizeWin;

TEST_CASE(Unit_SimpleJson, ParsePrimitives) {
    const std::string json = R"({
        "boolTrue": true,
        "boolFalse": false,
        "numberInt": 42,
        "numberFloat": 3.14,
        "stringVal": "hello world",
        "nullVal": null
    })";

    const JsonValue root = JsonValue::parse(json);
    ASSERT_TRUE(root.isObject());
    ASSERT_TRUE(root["boolTrue"].isBool());
    ASSERT_TRUE(root["boolTrue"].boolValue);
    ASSERT_TRUE(root["boolFalse"].isBool());
    ASSERT_FALSE(root["boolFalse"].boolValue);

    ASSERT_TRUE(root["numberInt"].isNumber());
    ASSERT_EQ(root["numberInt"].numberValue, 42.0);

    ASSERT_TRUE(root["stringVal"].isString());
    ASSERT_EQ(root["stringVal"].stringValue, "hello world");

    ASSERT_TRUE(root["nullVal"].isNull());
}

TEST_CASE(Unit_SimpleJson, NestedObjectsAndArrays) {
    const std::string json = R"({
        "name": "recommended",
        "tags": ["privacy", "safe", "windows"],
        "nested": {
            "enabled": true,
            "level": 1
        }
    })";

    const JsonValue root = JsonValue::parse(json);
    ASSERT_TRUE(root.isObject());
    ASSERT_EQ(root["name"].stringValue, "recommended");

    ASSERT_TRUE(root["tags"].isArray());
    ASSERT_EQ(root["tags"].arrayValue.size(), 3);
    ASSERT_EQ(root["tags"].arrayValue[0].stringValue, "privacy");
    ASSERT_EQ(root["tags"].arrayValue[1].stringValue, "safe");
    ASSERT_EQ(root["tags"].arrayValue[2].stringValue, "windows");

    ASSERT_TRUE(root["nested"].isObject());
    ASSERT_TRUE(root["nested"]["enabled"].boolValue);
    ASSERT_EQ(root["nested"]["level"].numberValue, 1.0);
}

TEST_CASE(Unit_SimpleJson, EscapedStrings) {
    const std::string json = R"({
        "escaped": "Line 1\nLine 2\t\"quoted\" \\ backslash"
    })";

    const JsonValue root = JsonValue::parse(json);
    ASSERT_TRUE(root.isObject());
    ASSERT_EQ(root["escaped"].stringValue, "Line 1\nLine 2\t\"quoted\" \\ backslash");
}

TEST_CASE(Unit_SimpleJson, RoundTripSerialization) {
    JsonValue root(JsonType::Object);
    root["app"] = "PrivatizeWin";
    root["version"] = 1;
    root["safe"] = true;

    JsonValue list(JsonType::Array);
    list.arrayValue.push_back(JsonValue("item1"));
    list.arrayValue.push_back(JsonValue("item2"));
    root["items"] = list;

    const std::string serialized = root.toString(2);
    const JsonValue parsed = JsonValue::parse(serialized);

    ASSERT_TRUE(parsed.isObject());
    ASSERT_EQ(parsed["app"].stringValue, "PrivatizeWin");
    ASSERT_EQ(parsed["version"].numberValue, 1.0);
    ASSERT_TRUE(parsed["safe"].boolValue);
    ASSERT_EQ(parsed["items"].arrayValue.size(), 2);
}

TEST_CASE(Unit_SimpleJson, RejectTruncatedJson) {
    bool caught = false;
    try {
        JsonValue::parse(R"({"incomplete": )");
    } catch (const std::exception&) {
        caught = true;
    }
    ASSERT_TRUE(caught);
}

TEST_CASE(Unit_SimpleJson, RejectTrailingGarbage) {
    bool caught = false;
    try {
        JsonValue::parse(R"({"valid": true} trailing_garbage)");
    } catch (const std::exception&) {
        caught = true;
    }
    ASSERT_TRUE(caught);
}

TEST_CASE(Unit_SimpleJson, UnicodeEscapeSequence) {
    const std::string json = R"({"text": "Hello \u0020 World \u00A9"})";
    const JsonValue root = JsonValue::parse(json);
    ASSERT_TRUE(root.isObject());
    ASSERT_EQ(root["text"].stringValue, "Hello   World \xC2\xA9");
}

TEST_CASE(Unit_SimpleJson, RejectInvalidEscapeSequence) {
    bool caught = false;
    try {
        JsonValue::parse(R"({"invalid": "bad\q"})");
    } catch (const std::exception&) {
        caught = true;
    }
    ASSERT_TRUE(caught);
}

TEST_CASE(Unit_SimpleJson, RejectInvalidUnicodeHex) {
    bool caught = false;
    try {
        JsonValue::parse(R"({"invalid": "\u00zz"})");
    } catch (const std::exception&) {
        caught = true;
    }
    ASSERT_TRUE(caught);
}

TEST_CASE(Unit_SimpleJson, RejectMalformedNumbers) {
    const std::vector<std::string> badNumbers = {
        R"({"n": 1+2})",
        R"({"n": 01})",
        R"({"n": 1.})",
        R"({"n": 1e})",
        R"({"n": 1e+})",
        R"({"n": --1})",
        R"({"n": +1})"
    };

    for (const auto& json : badNumbers) {
        bool caught = false;
        try {
            JsonValue::parse(json);
        } catch (const std::exception&) {
            caught = true;
        }
        ASSERT_TRUE(caught);
    }
}

TEST_CASE(Unit_SimpleJson, RejectUnescapedControlChars) {
    bool caught = false;
    try {
        // String literal with raw unescaped newline (0x0A)
        std::string rawJson = "{\"str\": \"line1\nline2\"}";
        JsonValue::parse(rawJson);
    } catch (const std::exception&) {
        caught = true;
    }
    ASSERT_TRUE(caught);
}

TEST_CASE(Unit_SimpleJson, RejectDeepNesting) {
    std::string deep;
    for (int i = 0; i < 70; ++i) deep += "{\"a\":";
    deep += "1";
    for (int i = 0; i < 70; ++i) deep += "}";

    bool caught = false;
    try {
        JsonValue::parse(deep);
    } catch (const std::exception&) {
        caught = true;
    }
    ASSERT_TRUE(caught);
}

TEST_CASE(Unit_SimpleJson, UnicodeSurrogatePairs) {
    // Emoji \uD83D\uDE00 -> 😀 (U+1F600) -> UTF-8 \xF0\x9F\x98\x80
    const std::string json = R"({"emoji": "Hello \uD83D\uDE00 World!"})";
    const JsonValue root = JsonValue::parse(json);
    ASSERT_TRUE(root.isObject());
    ASSERT_EQ(root["emoji"].stringValue, "Hello \xF0\x9F\x98\x80 World!");
}

TEST_CASE(Unit_SimpleJson, RejectUnpairedSurrogates) {
    // Unpaired high surrogate followed by regular char
    bool caught1 = false;
    try {
        JsonValue::parse(R"({"bad": "\uD83D abc"})");
    } catch (const std::exception&) {
        caught1 = true;
    }
    ASSERT_TRUE(caught1);

    // Unpaired low surrogate
    bool caught2 = false;
    try {
        JsonValue::parse(R"({"bad": "\uDE00"})");
    } catch (const std::exception&) {
        caught2 = true;
    }
    ASSERT_TRUE(caught2);
}

TEST_CASE(Unit_SimpleJson, EscapeAndSerializeControlChars) {
    JsonValue root(JsonType::Object);
    std::string specialChars = "Quote: \", Backslash: \\, Backspace: \b, Formfeed: \f, Newline: \n, Return: \r, Tab: \t, Ctrl: \x01\x1F";
    root["special"] = specialChars;

    const std::string serialized = root.toString(0);
    // Verify that \b, \f, and \u0001 are properly escaped in the JSON output
    ASSERT_TRUE(serialized.find("\\b") != std::string::npos);
    ASSERT_TRUE(serialized.find("\\f") != std::string::npos);
    ASSERT_TRUE(serialized.find("\\u0001") != std::string::npos);
    ASSERT_TRUE(serialized.find("\\u001f") != std::string::npos || serialized.find("\\u001F") != std::string::npos);

    // Round-trip parse
    const JsonValue roundTrip = JsonValue::parse(serialized);
    ASSERT_TRUE(roundTrip.isObject());
    ASSERT_EQ(roundTrip["special"].stringValue, specialChars);
}
