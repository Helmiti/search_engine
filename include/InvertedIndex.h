#pragma once
#include <cstddef>
#include <map>
#include <string>
#include <vector>

struct Entry {
    std::size_t doc_id = 0;
    std::size_t count = 0;
    bool operator==(const Entry& other) const {
        return doc_id == other.doc_id && count == other.count;
    }
};

class InvertedIndex {
public:
    void UpdateDocumentBase(std::vector<std::string> input_docs);
    std::vector<Entry> GetWordCount(const std::string& word) const;
    std::size_t DocumentCount() const;
private:
    std::vector<std::string> docs_;
    std::map<std::string, std::vector<Entry>> freq_dictionary_;
};
