#include "ConverterJSON.h"
#include "InvertedIndex.h"
#include "MiniJSON.h"
#include "SearchServer.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

void test_index() {
    InvertedIndex idx;
    idx.UpdateDocumentBase({"milk sugar salt", "milk a milk b milk c milk d"});
    assert((idx.GetWordCount("milk") == std::vector<Entry>{{0,1}, {1,4}}));
    assert((idx.GetWordCount("sugar") == std::vector<Entry>{{0,1}}));
    assert(idx.GetWordCount("unknown").empty());
    idx.UpdateDocumentBase({"new new"});
    assert(idx.GetWordCount("milk").empty());
    assert((idx.GetWordCount("new") == std::vector<Entry>{{0,2}}));
}
void test_search() {
    InvertedIndex idx;
    idx.UpdateDocumentBase({"milk milk milk milk water water water", "milk water water",
                            "milk milk milk milk milk water water water water water", "americano cappuccino"});
    SearchServer server(idx);
    const auto result = server.search({"milk water", "sugar", "milk milk water", "cappuccino", ""});
    assert(result.size() == 5);
    assert(result[0].size() == 3);
    assert(result[0][0].doc_id == 2 && result[0][0].rank == 1.0f);
    assert(result[0][1].doc_id == 0 && std::abs(result[0][1].rank - 0.7f) < 0.00001f);
    assert(result[0][2].doc_id == 1 && std::abs(result[0][2].rank - 0.3f) < 0.00001f);
    assert(result[1].empty());
    assert(result[2] == result[0]);
    assert(result[3].size() == 1 && result[3][0].doc_id == 3);
    assert(result[4].empty());
    assert(server.search({"milk nonexistent"})[0].empty());
}
void test_json() {
    const auto data = mini_json::Value::parse("{\"a\":[true,null,3,\"escaped \\\" quote\"],\"unicode\":\"\\u041f\"}");
    assert(data.at("a").as_array().size() == 4);
    assert(data.at("a").as_array()[0].as_bool());
    assert(data.at("a").as_array()[2].as_number() == 3);
    assert(mini_json::Value::parse(data.dump()).at("unicode").as_string() == "\xD0\x9F");
    for (const std::string invalid : {"{", "[1,]", "{\"a\":1,}", "NaN", "{\"a\":1,\"a\":2}"}) {
        bool rejected = false;
        try { (void)mini_json::Value::parse(invalid); } catch (const std::runtime_error&) { rejected = true; }
        assert(rejected);
    }
}
void test_converter() {
    namespace fs = std::filesystem;
    const fs::path folder = fs::temp_directory_path() / "search_engine_test_case";
    fs::create_directories(folder);
    {
        std::ofstream(folder / "config.json") << R"({"config":{"name":"UnitTest","version":"0.1","max_responses":2},"files":["a.txt","missing.txt","b.txt"]})";
        std::ofstream(folder / "requests.json") << R"({"requests":["milk water","nope"]})";
        std::ofstream(folder / "a.txt") << "milk water";
        std::ofstream(folder / "b.txt") << "milk milk water";
    }
    ConverterJSON converter(folder.string());
    assert(converter.GetEngineName() == "UnitTest");
    assert(converter.GetResponsesLimit() == 2);
    assert(converter.GetRequests().size() == 2);
    const auto docs = converter.GetTextDocuments();
    assert(docs.size() == 3 && docs[1].empty() && docs[2] == "milk milk water");
    converter.putAnswers({{{2,1.0f},{0,0.6666667f},{1,0.2f}}, {}});
    std::ifstream input(folder / "answers.json");
    std::string contents((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    const auto result = mini_json::Value::parse(contents);
    assert(result.at("answers").at("request001").at("result").as_string() == "true");
    assert(result.at("answers").at("request001").at("relevance").as_array().size() == 2);
    assert(result.at("answers").at("request001").at("relevance").as_array()[0].at("docid").as_number() == 2);
    assert(result.at("answers").at("request002").at("result").as_string() == "false");
    input.close();
    fs::remove_all(folder);
}
int main() {
    test_index(); test_search(); test_json(); test_converter();
    std::cout << "All offline tests passed!\n";
}
