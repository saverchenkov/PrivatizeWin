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
