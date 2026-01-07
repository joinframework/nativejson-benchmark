#include "../test.h"

#include <string>
#include <sstream>

#define ARDUINOJSON_ENABLE_STD_STREAM 1
#include "ArduinoJson/src/ArduinoJson.hpp"

using namespace ArduinoJson;

static void GenStat(Stat& stat, ArduinoJson::JsonVariantConst v) {
    if (v.is<JsonArrayConst>()) {
        JsonArrayConst a = v.as<JsonArrayConst>();
        for (JsonVariantConst v : a)
            GenStat(stat, v);
        stat.arrayCount++;
        stat.elementCount += a.size();
    }
    else if (v.is<JsonObjectConst>()) {
        JsonObjectConst o = v.as<JsonObjectConst>();
        for (JsonPairConst kv : o) {
            GenStat(stat, kv.value());
            stat.stringLength += strlen(kv.key().c_str());
        }
        stat.objectCount++;
        stat.memberCount += o.size();
        stat.stringCount += o.size();
    }
    else if (v.is<const char*>()) {
        if (v.as<const char*>()) {
            stat.stringCount++;
            stat.stringLength += strlen(v.as<const char*>());
        }
        else
            stat.nullCount++; // JSON null value is treat as string null pointer
    }
    else if (v.is<long>() || v.is<double>())
        stat.numberCount++;
    else if (v.is<bool>()) {
        if ((bool)v)
            stat.trueCount++;
        else
            stat.falseCount++;
    }
    else {
        stat.nullCount++;
    }
}

class ArduinojsonParseResult : public ParseResultBase {
public:
    JsonDocument doc;
};

class ArduinojsonStringResult : public StringResultBase {
public:
    virtual const char* c_str() const { return s.c_str(); }

    std::string s;
};

class ArduinojsonTest : public TestBase {
public:
#if TEST_INFO
    virtual const char* GetName() const { return "ArduinoJson (C++)"; }
    virtual const char* GetFilename() const { return __FILE__; }
#endif

#if TEST_PARSE
    virtual ParseResultBase* Parse(const char* json, size_t length) const {
        ArduinojsonParseResult* pr = new ArduinojsonParseResult;
        DeserializationError err = deserializeJson(pr->doc, json, length);
        if (err) {
            delete pr;
            return 0;
        }
        return pr;
    }
#endif

#if TEST_STRINGIFY
    virtual StringResultBase* Stringify(const ParseResultBase* parseResult) const {
        const ArduinojsonParseResult* pr = static_cast<const ArduinojsonParseResult*>(parseResult);
        ArduinojsonStringResult* sr = new ArduinojsonStringResult;
        std::ostringstream os;
        serializeJson(pr->doc, os);
        sr->s = os.str();
        return sr;
    }
#endif

#if TEST_PRETTIFY
    virtual StringResultBase* Prettify(const ParseResultBase* parseResult) const {
        const ArduinojsonParseResult* pr = static_cast<const ArduinojsonParseResult*>(parseResult);
        ArduinojsonStringResult* sr = new ArduinojsonStringResult;
        std::ostringstream os;
        serializeJsonPretty(pr->doc, os);
        sr->s = os.str();
        return sr;
    }
#endif

#if TEST_STATISTICS
    virtual bool Statistics(const ParseResultBase* parseResult, Stat* stat) const {
        const ArduinojsonParseResult* pr = static_cast<const ArduinojsonParseResult*>(parseResult);
        memset(stat, 0, sizeof(Stat));
        GenStat(*stat, pr->doc.as<JsonVariantConst>());
        return true;
    }
#endif

#if TEST_CONFORMANCE
    virtual bool ParseDouble(const char* json, double* d) const {
        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, json);
        if (err) {
            return false;
        }
        *d = doc[0].as<double>();
        return true;
    }

    virtual bool ParseString(const char* json, std::string& s) const {
        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, json);
        if (err) {
            return false;
        }
        s = doc[0].as<std::string>();
        return true;
    }
#endif
};

REGISTER_TEST(ArduinojsonTest);
