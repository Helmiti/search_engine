#pragma once
#include <cstddef>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>


namespace mini_json {
class Value {
public:
    enum class Type { Null, Boolean, Number, String, Array, Object };
    using Array = std::vector<Value>;
    using Object = std::map<std::string, Value>;

    Value();
    Value(bool value);
    Value(int value);
    Value(double value);
    Value(const char* value);
    Value(std::string value);
    Value(Array value);
    Value(Object value);
    static Value array();
    static Value object();
    Type type() const;
    bool is_object() const;
    bool is_array() const;
    bool contains(const std::string& key) const;
    const Value& at(const std::string& key) const;
    const Array& as_array() const;
    const Object& as_object() const;
    const std::string& as_string() const;
    double as_number() const;
    bool as_bool() const;
    Value& operator[](const std::string& key);
    void push_back(Value value);
    std::string dump(int indent = 2) const;
    static Value parse(const std::string& input);

private:
    Type type_ = Type::Null;
    bool boolean_ = false;
    double number_ = 0.0;
    std::string string_;
    Array array_;
    Object object_;
    std::string dump_at(int indent, int depth) const;
};
}
