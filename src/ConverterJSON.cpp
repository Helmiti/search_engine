#include "ConverterJSON.h"
#include "MiniJSON.h"
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace {
std::string read_all(const std::string& path, const std::string& missing_message) {
    std::ifstream file(path, std::ios::binary);
    if (!file) throw std::runtime_error(missing_message);
    std::ostringstream text;
    text << file.rdbuf();
    if (file.bad()) throw std::runtime_error("Cannot read " + path);
    return text.str();
}
mini_json::Value load_config(const std::string& path) {
    const auto config = mini_json::Value::parse(read_all(path, "config file is missing"));
    if (!config.is_object() || !config.contains("config") || !config.at("config").is_object() ||
        config.at("config").as_object().empty()) throw std::runtime_error("config file is empty");
    if (config.at("config").contains("version") && config.at("config").at("version").as_string() != "0.1")
        throw std::runtime_error("config.json has incorrect file version");
    return config;
}
std::string read_document(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) throw std::runtime_error("Cannot open document: " + path);
    std::ostringstream out;
    out << file.rdbuf();
    if (file.bad()) throw std::runtime_error("Cannot read document: " + path);
    return out.str();
}
}
ConverterJSON::ConverterJSON(std::string working_directory) : working_directory_(std::move(working_directory)) {}
std::string ConverterJSON::path(const std::string& file) const {
    if (working_directory_.empty() || working_directory_ == ".") return file;
    const char end = working_directory_.back();
    return working_directory_ + ((end == '/' || end == '\\') ? "" : "/") + file;
}
std::string ConverterJSON::GetEngineName() {
    const auto config = load_config(path("config.json"));
    const auto& data = config.at("config");
    return data.contains("name") ? data.at("name").as_string() : "SearchEngine";
}
int ConverterJSON::GetResponsesLimit() {
    const auto config = load_config(path("config.json"));
    const auto& settings = config.at("config");
    if (!settings.contains("max_responses")) return 5;
    const double value = settings.at("max_responses").as_number();
    if (!std::isfinite(value) || value < 0 || value > std::numeric_limits<int>::max() || std::floor(value) != value)
        throw std::runtime_error("max_responses must be a non-negative integer");
    return static_cast<int>(value);
}
std::vector<std::string> ConverterJSON::GetTextDocuments() {
    const auto config = load_config(path("config.json"));
    std::vector<std::string> docs;
    if (!config.contains("files") || !config.at("files").is_array())
        throw std::runtime_error("config.json: files must be an array");
    for (const auto& file : config.at("files").as_array()) {
        const std::string file_name = file.as_string();
        try {

            docs.push_back(read_document(path(file_name)));
        } catch (const std::exception& ex) {
            std::cerr << "Warning: " << ex.what() << '\n';
            docs.emplace_back();
        }
    }
    return docs;
}
std::vector<std::string> ConverterJSON::GetRequests() {
    const auto requests = mini_json::Value::parse(read_all(path("requests.json"), "requests file is missing"));
    if (!requests.is_object() || !requests.contains("requests") || !requests.at("requests").is_array())
        throw std::runtime_error("requests.json: requests must be an array");
    std::vector<std::string> output;
    for (const auto& query : requests.at("requests").as_array()) {
        if (output.size() >= 1000) throw std::runtime_error("Too many requests (max 1000)");
        output.push_back(query.as_string());
    }
    return output;
}
void ConverterJSON::putAnswers(const std::vector<std::vector<std::pair<int, float>>>& answers) {
    const int limit = GetResponsesLimit();
    mini_json::Value root = mini_json::Value::object();
    mini_json::Value results = mini_json::Value::object();
    for (std::size_t i = 0; i < answers.size(); ++i) {
        std::ostringstream key;
        key << "request" << std::setfill('0') << std::setw(3) << i + 1;
        mini_json::Value item = mini_json::Value::object();
        const std::size_t n = std::min(answers[i].size(), static_cast<std::size_t>(limit));
        item["result"] = n > 0 ? "true" : "false";
        if (n > 0) {
            mini_json::Value relevance = mini_json::Value::array();
            for (std::size_t j = 0; j < n; ++j) {
                mini_json::Value entry = mini_json::Value::object();
                entry["docid"] = answers[i][j].first;
                entry["rank"] = std::round(static_cast<double>(answers[i][j].second) * 1000000.0) / 1000000.0;
                relevance.push_back(std::move(entry));
            }
            item["relevance"] = std::move(relevance);
        }
        results[key.str()] = std::move(item);
    }
    root["answers"] = std::move(results);
    std::ofstream out(path("answers.json"), std::ios::binary | std::ios::trunc);
    if (!out) throw std::runtime_error("Cannot write answers.json");
    out << root.dump(2) << '\n';
    if (!out) throw std::runtime_error("Cannot finish writing answers.json");
}
