#include "InvertedIndex.h"
#include <future>
#include <map>
#include <sstream>
#include <utility>

namespace {
std::map<std::string, std::size_t> count_words(const std::string& document) {
    std::istringstream stream(document);
    std::string word;
    std::map<std::string, std::size_t> result;
    while (stream >> word) ++result[word];
    return result;
}
}
void InvertedIndex::UpdateDocumentBase(std::vector<std::string> input_docs) {
    docs_ = std::move(input_docs);
    freq_dictionary_.clear();


    std::vector<std::future<std::map<std::string, std::size_t>>> tasks;
    tasks.reserve(docs_.size());
    for (const auto& document : docs_) {
        tasks.push_back(std::async(std::launch::async, count_words, document));
    }
    for (std::size_t doc_id = 0; doc_id < tasks.size(); ++doc_id) {
        for (const auto& [word, count] : tasks[doc_id].get()) {
            freq_dictionary_[word].push_back({doc_id, count});
        }
    }
}
std::vector<Entry> InvertedIndex::GetWordCount(const std::string& word) const {
    const auto it = freq_dictionary_.find(word);
    return it == freq_dictionary_.end() ? std::vector<Entry>{} : it->second;
}
std::size_t InvertedIndex::DocumentCount() const { return docs_.size(); }
