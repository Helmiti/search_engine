#include "SearchServer.h"
#include <algorithm>
#include <map>
#include <set>
#include <sstream>
#include <utility>

std::vector<std::vector<RelativeIndex>> SearchServer::search(const std::vector<std::string>& queries_input) const {
    std::vector<std::vector<RelativeIndex>> responses;
    responses.reserve(queries_input.size());
    for (const auto& query : queries_input) {
        std::istringstream input(query);
        std::set<std::string> unique_words;
        std::string word;
        while (input >> word) unique_words.insert(word);
        if (unique_words.empty()) { responses.emplace_back(); continue; }

        struct Posting { std::string word; std::vector<Entry> entries; std::size_t total = 0; };
        std::vector<Posting> postings;
        bool missing = false;
        for (const auto& term : unique_words) {
            auto entries = index_.GetWordCount(term);
            if (entries.empty()) { missing = true; break; }
            std::size_t total = 0;
            for (const auto& entry : entries) total += entry.count;
            postings.push_back({term, std::move(entries), total});
        }
        if (missing) { responses.emplace_back(); continue; }
        std::sort(postings.begin(), postings.end(), [](const Posting& a, const Posting& b) {
            return a.total != b.total ? a.total < b.total : a.word < b.word;
        });


        std::map<std::size_t, std::size_t> matched;
        for (const auto& entry : postings.front().entries) matched[entry.doc_id] = entry.count;
        for (std::size_t i = 1; i < postings.size() && !matched.empty(); ++i) {
            std::map<std::size_t, std::size_t> next;
            for (const auto& entry : postings[i].entries) {
                const auto it = matched.find(entry.doc_id);
                if (it != matched.end()) next[entry.doc_id] = it->second + entry.count;
            }
            matched.swap(next);
        }
        std::vector<RelativeIndex> result;
        std::size_t max_score = 0;
        for (const auto& [doc_id, score] : matched) max_score = std::max(max_score, score);
        for (const auto& [doc_id, score] : matched) {
            result.push_back({doc_id, max_score ? static_cast<float>(score) / static_cast<float>(max_score) : 0.0f});
        }
        std::sort(result.begin(), result.end(), [](const RelativeIndex& a, const RelativeIndex& b) {
            return a.rank != b.rank ? a.rank > b.rank : a.doc_id < b.doc_id;
        });
        responses.push_back(std::move(result));
    }
    return responses;
}
