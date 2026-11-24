#pragma once
#include <string>
#include <utility>
#include <vector>

class ConverterJSON {
public:
    explicit ConverterJSON(std::string working_directory = ".");
    std::vector<std::string> GetTextDocuments();
    int GetResponsesLimit();
    std::vector<std::string> GetRequests();
    void putAnswers(const std::vector<std::vector<std::pair<int, float>>>& answers);
    std::string GetEngineName();

private:
    std::string working_directory_;
    std::string path(const std::string& file) const;
};
