#if defined(__GNUC__)

#include "../test.h"

#include "join/core/src/error.cpp"
#include "join/data/src/value.cpp"
#include "join/data/src/sax.cpp"
#include "join/data/include/join/pack.hpp"
#include "join/data/include/join/json.hpp"
#include "join/data/src/json.cpp"

#include <sstream>
#include <stack>

using join::Value;
using join::JsonReader;
using join::JsonWriter;

class StatHandler : public JsonReader
{
public:
    StatHandler (Value& root, Stat& stat) 
    : JsonReader (root)
    , _stat(stat)
    {}

protected:
    virtual int setNull () override                       { ++_stat.nullCount; setVal (); return 0; }
    virtual int setBool (bool b) override                 { if (b) ++_stat.trueCount; else ++_stat.falseCount; setVal (); return 0; }
    virtual int setInt (int32_t) override                 { ++_stat.numberCount; setVal (); return 0; }
    virtual int setUint (uint32_t) override               { ++_stat.numberCount; setVal (); return 0; }
    virtual int setInt64 (int64_t) override               { ++_stat.numberCount; setVal (); return 0; }
    virtual int setUint64 (uint64_t) override             { ++_stat.numberCount; setVal (); return 0; }
    virtual int setDouble (double) override               { ++_stat.numberCount; setVal (); return 0; }
    virtual int setString (const std::string& s) override { ++_stat.stringCount; _stat.stringLength += s.size (); setVal (); return 0; }
    virtual int startArray (uint32_t) override            { ++_stat.arrayCount; setVal (); _stack.push (0); return 0; }
    virtual int stopArray () override                     { _stat.elementCount += _stack.top (); _stack.pop (); return 0; }
    virtual int startObject (uint32_t) override           { ++_stat.objectCount; setVal (); _stack.push (0); return 0; }
    virtual int setKey (const std::string& s) override    { ++_stat.stringCount; _stat.stringLength += s.size (); return 0; }
    virtual int stopObject () override                    { _stat.memberCount += _stack.top (); _stack.pop (); return 0; }

private:
    void setVal () { if (_stack.size ()) ++_stack.top (); }

    std::stack <int> _stack;
    Stat& _stat;
};

static void GenStat (Stat& stat, const Value& v) 
{
    switch (v.index ()) {
        case Value::ArrayValue:
            for (auto& element : v.getArray ())
                GenStat (stat, element);
            stat.arrayCount++;
            stat.elementCount += v.size ();
            break;

        case Value::ObjectValue:
            for (auto& element : v.getObject ()) {
                GenStat (stat, element.second);
                stat.stringLength += element.first.size ();
            }
            stat.objectCount++;
            stat.memberCount += v.size ();
            stat.stringCount += v.size ();
            break;

        case Value::String:
            stat.stringCount++;
            stat.stringLength += v.size ();
            break;

        case Value::Integer:
        case Value::Unsigned:
        case Value::Integer64:
        case Value::Unsigned64:
        case Value::Real:
            stat.numberCount++;
            break;

        case Value::Boolean:
            if (v.isTrue ())
                stat.trueCount++;
            else
                stat.falseCount++;
            break;

        case Value::Null:
            stat.nullCount++;
            break;
    }
}

class JoinParseResult : public ParseResultBase {
public:
    Value root;
};

class JoinStringResult : public StringResultBase {
public:
    virtual const char* c_str () const { 
        return s.c_str ();
    }

    std::string s;
};

class JoinTest : public TestBase {
public:
#if TEST_INFO
    virtual const char* GetName () const { 
        return "join (C++14)"; 
    }

    virtual const char* GetFilename () const { 
        return __FILE__;
    }
#endif

#if TEST_PARSE
    virtual ParseResultBase* Parse (const char* j, size_t length) const {
        JoinParseResult* pr = new JoinParseResult;
        if (pr->root.deserialize <JsonReader> (j, length) == -1) {
            delete pr;
            return 0;
        }
        return pr;
    }
#endif

#if TEST_STRINGIFY
    virtual StringResultBase* Stringify (const ParseResultBase* parseResult) const {
        const JoinParseResult* pr = static_cast <const JoinParseResult*> (parseResult);
        JoinStringResult* sr = new JoinStringResult;
        std::stringstream ss;
        pr->root.serialize <JsonWriter> (ss);
        sr->s = ss.str ();
        return sr;
    }
#endif

#if TEST_PRETTIFY
    virtual StringResultBase* Prettify (const ParseResultBase* parseResult) const {
        const JoinParseResult* pr = static_cast <const JoinParseResult*> (parseResult);
        JoinStringResult* sr = new JoinStringResult;
        std::stringstream ss;
        JsonWriter writer (ss, 2);
        writer.serialize (pr->root);
        sr->s = ss.str ();
        return sr;
    }
#endif

#if TEST_STATISTICS
    virtual bool Statistics (const ParseResultBase* parseResult, Stat* stat) const {
        const JoinParseResult* pr = static_cast <const JoinParseResult*> (parseResult);
        memset (stat, 0, sizeof (Stat));
        GenStat (*stat, pr->root);
        return true;
    }
#endif

#if TEST_SAXROUNDTRIP
    virtual StringResultBase* SaxRoundtrip(const char* json, size_t length) const {
        Value value;
        if (value.deserialize <JsonReader> (json, length) == -1) {
            return 0;
        }
        JoinStringResult* sr = new JoinStringResult;
        std::stringstream ss;
        if (value.serialize <JsonWriter> (ss) == -1) {
            delete sr;
            return 0;
        }
        sr->s = ss.str ();
        return sr;
    }
#endif

#if TEST_SAXSTATISTICS
    virtual bool SaxStatistics(const char* json, size_t length, Stat* stat) const {
        Value value;
        memset (stat, 0, sizeof (Stat));
        StatHandler reader (value, *stat);
        return reader.deserialize (json, length) == 0;
    }
#endif

#if TEST_CONFORMANCE
    virtual bool ParseDouble (const char* j, double* d) const {
        Value root;
        if (root.deserialize <JsonReader> (j) != -1) {
            *d = root[0].getDouble ();
            return true;
        }
        return false;
    }

    virtual bool ParseString (const char* j, std::string& s) const {
        Value root;
        if (root.deserialize <JsonReader> (j) != -1) {
            s = root[0].getString ();
            return true;
        }
        return false;
    }
#endif
};

REGISTER_TEST(JoinTest);

#endif
