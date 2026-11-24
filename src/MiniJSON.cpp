#include "MiniJSON.h"
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <utility>

namespace mini_json {
Value::Value() = default;
Value::Value(bool value) : type_(Type::Boolean), boolean_(value) {}
Value::Value(int value) : type_(Type::Number), number_(value) {}
Value::Value(double value) : type_(Type::Number), number_(value) {}
Value::Value(const char* value) : Value(std::string(value)) {}
Value::Value(std::string value) : type_(Type::String), string_(std::move(value)) {}
Value::Value(Array value) : type_(Type::Array), array_(std::move(value)) {}
Value::Value(Object value) : type_(Type::Object), object_(std::move(value)) {}
Value Value::array() { return Value(Array{}); }
Value Value::object() { return Value(Object{}); }
Value::Type Value::type() const { return type_; }
bool Value::is_object() const { return type_ == Type::Object; }
bool Value::is_array() const { return type_ == Type::Array; }
bool Value::contains(const std::string& key) const { return is_object() && object_.find(key) != object_.end(); }
const Value& Value::at(const std::string& key) const {
    if (!is_object()) throw std::runtime_error("JSON value is not an object");
    const auto it = object_.find(key);
    if (it == object_.end()) throw std::runtime_error("Missing JSON key: " + key);
    return it->second;
}
const Value::Array& Value::as_array() const {
    if (!is_array()) throw std::runtime_error("JSON value is not an array");
    return array_;
}
const Value::Object& Value::as_object() const {
    if (!is_object()) throw std::runtime_error("JSON value is not an object");
    return object_;
}
const std::string& Value::as_string() const {
    if (type_ != Type::String) throw std::runtime_error("JSON value is not a string");
    return string_;
}
double Value::as_number() const {
    if (type_ != Type::Number) throw std::runtime_error("JSON value is not a number");
    return number_;
}
bool Value::as_bool() const {
    if (type_ != Type::Boolean) throw std::runtime_error("JSON value is not a boolean");
    return boolean_;
}
Value& Value::operator[](const std::string& key) {
    if (type_ == Type::Null) type_ = Type::Object;
    if (!is_object()) throw std::runtime_error("JSON value is not an object");
    return object_[key];
}
void Value::push_back(Value value) {
    if (type_ == Type::Null) type_ = Type::Array;
    if (!is_array()) throw std::runtime_error("JSON value is not an array");
    array_.push_back(std::move(value));
}
namespace {
std::string escape(const std::string& input) {
    std::ostringstream out;
    for (unsigned char c : input) {
        switch (c) {
            case '\"': out << "\\\""; break;
            case '\\': out << "\\\\"; break;
            case '\b': out << "\\b"; break;
            case '\f': out << "\\f"; break;
            case '\n': out << "\\n"; break;
            case '\r': out << "\\r"; break;
            case '\t': out << "\\t"; break;
            default:
                if (c < 32) out << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(c) << std::dec;
                else out << static_cast<char>(c);
        }
    }
    return out.str();
}
void append_utf8(std::string& output, std::uint32_t point) {
    if (point <= 0x7F) output += static_cast<char>(point);
    else if (point <= 0x7FF) {
        output += static_cast<char>(0xC0 | (point >> 6));
        output += static_cast<char>(0x80 | (point & 0x3F));
    } else if (point <= 0xFFFF) {
        output += static_cast<char>(0xE0 | (point >> 12));
        output += static_cast<char>(0x80 | ((point >> 6) & 0x3F));
        output += static_cast<char>(0x80 | (point & 0x3F));
    } else {
        output += static_cast<char>(0xF0 | (point >> 18));
        output += static_cast<char>(0x80 | ((point >> 12) & 0x3F));
        output += static_cast<char>(0x80 | ((point >> 6) & 0x3F));
        output += static_cast<char>(0x80 | (point & 0x3F));
    }
}
class Parser {
public:
    explicit Parser(const std::string& source) : source_(source) {}
    Value parse() {
        skip();
        Value result = read_value();
        skip();
        if (position_ != source_.size()) fail("Trailing characters");
        return result;
    }
private:
    const std::string& source_;
    std::size_t position_ = 0;
    [[noreturn]] void fail(const std::string& reason) const {
        throw std::runtime_error("JSON parse error at byte " + std::to_string(position_) + ": " + reason);
    }
    void skip() {
        while (position_ < source_.size() && (source_[position_] == ' ' || source_[position_] == '\n' ||
               source_[position_] == '\r' || source_[position_] == '\t')) ++position_;
    }
    bool take(char c) {
        skip();
        if (position_ < source_.size() && source_[position_] == c) { ++position_; return true; }
        return false;
    }
    void expect(char c) { if (!take(c)) fail(std::string("Expected '") + c + "'"); }
    std::uint32_t hex4() {
        if (position_ + 4 > source_.size()) fail("Incomplete unicode escape");
        std::uint32_t value = 0;
        for (int i = 0; i < 4; ++i) {
            const char c = source_[position_++];
            value <<= 4;
            if (c >= '0' && c <= '9') value += c - '0';
            else if (c >= 'a' && c <= 'f') value += c - 'a' + 10;
            else if (c >= 'A' && c <= 'F') value += c - 'A' + 10;
            else fail("Bad unicode escape");
        }
        return value;
    }
    std::string read_string() {
        expect('"');
        std::string value;
        while (position_ < source_.size()) {
            const unsigned char c = static_cast<unsigned char>(source_[position_++]);
            if (c == '"') return value;
            if (c < 32) fail("Control character in string");
            if (c != '\\') { value += static_cast<char>(c); continue; }
            if (position_ == source_.size()) fail("Incomplete escape");
            const char e = source_[position_++];
            switch (e) {
                case '"': value += '"'; break;
                case '\\': value += '\\'; break;
                case '/': value += '/'; break;
                case 'b': value += '\b'; break;
                case 'f': value += '\f'; break;
                case 'n': value += '\n'; break;
                case 'r': value += '\r'; break;
                case 't': value += '\t'; break;
                case 'u': {
                    std::uint32_t point = hex4();
                    if (point >= 0xD800 && point <= 0xDBFF) {
                        if (position_ + 2 > source_.size() || source_.substr(position_, 2) != "\\u") fail("Missing unicode surrogate");
                        position_ += 2;
                        const std::uint32_t low = hex4();
                        if (low < 0xDC00 || low > 0xDFFF) fail("Bad unicode surrogate");
                        point = 0x10000 + ((point - 0xD800) << 10) + low - 0xDC00;
                    } else if (point >= 0xDC00 && point <= 0xDFFF) fail("Unpaired unicode surrogate");
                    append_utf8(value, point);
                    break;
                }
                default: fail("Invalid string escape");
            }
        }
        fail("Unterminated string");
    }
    Value read_value() {
        skip();
        if (position_ >= source_.size()) fail("Missing value");
        const char c = source_[position_];
        if (c == '"') return Value(read_string());
        if (c == '{') {
            ++position_;
            Value obj = Value::object();
            if (take('}')) return obj;
            do {
                skip();
                if (position_ == source_.size() || source_[position_] != '"') fail("Expected object key");
                const std::string key = read_string();
                expect(':');
                if (obj.contains(key)) fail("Duplicate object key");
                obj[key] = read_value();
                if (take('}')) return obj;
                expect(',');
            } while (true);
        }
        if (c == '[') {
            ++position_;
            Value arr = Value::array();
            if (take(']')) return arr;
            do {
                arr.push_back(read_value());
                if (take(']')) return arr;
                expect(',');
            } while (true);
        }
        for (const auto& keyword : {"true", "false", "null"}) {
            const std::string literal(keyword);
            if (source_.compare(position_, literal.size(), literal) == 0) {
                position_ += literal.size();
                if (literal == "null") return Value();
                return Value(literal == "true");
            }
        }
        const std::size_t start = position_;
        if (source_[position_] == '-') ++position_;
        if (position_ >= source_.size()) fail("Invalid number");
        if (source_[position_] == '0') ++position_;
        else {
            if (source_[position_] < '1' || source_[position_] > '9') fail("Invalid number");
            while (position_ < source_.size() && source_[position_] >= '0' && source_[position_] <= '9') ++position_;
        }
        if (position_ < source_.size() && source_[position_] == '.') {
            ++position_;
            if (position_ == source_.size() || source_[position_] < '0' || source_[position_] > '9') fail("Invalid fraction");
            while (position_ < source_.size() && source_[position_] >= '0' && source_[position_] <= '9') ++position_;
        }
        if (position_ < source_.size() && (source_[position_] == 'e' || source_[position_] == 'E')) {
            ++position_;
            if (position_ < source_.size() && (source_[position_] == '+' || source_[position_] == '-')) ++position_;
            if (position_ == source_.size() || source_[position_] < '0' || source_[position_] > '9') fail("Invalid exponent");
            while (position_ < source_.size() && source_[position_] >= '0' && source_[position_] <= '9') ++position_;
        }
        double value = 0;
        try { value = std::stod(source_.substr(start, position_ - start)); }
        catch (const std::exception&) { fail("Invalid number"); }
        if (!std::isfinite(value)) fail("Number out of range");
        return Value(value);
    }
};
}
std::string Value::dump_at(int indent, int depth) const {
    const bool pretty = indent > 0;
    const std::string padding(pretty ? static_cast<std::size_t>(indent * depth) : 0, ' ');
    const std::string next_padding(pretty ? static_cast<std::size_t>(indent * (depth + 1)) : 0, ' ');
    const std::string separator = pretty ? "\n" : "";
    switch (type_) {
        case Type::Null: return "null";
        case Type::Boolean: return boolean_ ? "true" : "false";
        case Type::Number: {
            std::ostringstream out;
            out << std::setprecision(10) << number_;
            return out.str();
        }
        case Type::String: return "\"" + escape(string_) + "\"";
        case Type::Array: {
            if (array_.empty()) return "[]";
            std::string result = "[" + separator;
            for (std::size_t i = 0; i < array_.size(); ++i) {
                result += next_padding + array_[i].dump_at(indent, depth + 1);
                if (i + 1 < array_.size()) result += ',';
                result += separator;
            }
            return result + padding + "]";
        }
        case Type::Object: {
            if (object_.empty()) return "{}";
            std::string result = "{" + separator;
            std::size_t i = 0;
            for (const auto& entry : object_) {
                result += next_padding + "\"" + escape(entry.first) + "\":" + (pretty ? " " : "") + entry.second.dump_at(indent, depth + 1);
                if (++i < object_.size()) result += ',';
                result += separator;
            }
            return result + padding + "}";
        }
    }
    throw std::runtime_error("Invalid JSON type");
}
std::string Value::dump(int indent) const { return dump_at(indent, 0); }
Value Value::parse(const std::string& input) { return Parser(input).parse(); }
}
