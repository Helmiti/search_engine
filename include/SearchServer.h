#pragma once
#include "InvertedIndex.h"
#include <cstddef>
#include <string>
#include <vector>

struct RelativeIndex {
    std::size_t doc_id = 0;
    float rank = 0;
    bool operator==(const RelativeIndex& other) const {
        return doc_id == other.doc_id && rank == other.rank;
    }
};
class SearchServer {
public:
    explicit SearchServer(InvertedIndex& index) : index_(index) {}
    std::vector<std::vector<RelativeIndex>> search(const std::vector<std::string>& queries_input) const;
private:
    InvertedIndex& index_;
};
