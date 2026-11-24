#include "ConverterJSON.h"
#include "InvertedIndex.h"
#include "SearchServer.h"
#include <exception>
#include <iostream>
#include <utility>
#include <vector>

int main(int argc, char* argv[]) {
    try {

        const std::string workdir = argc >= 2 ? argv[1] : ".";
        ConverterJSON converter(workdir);
        std::cout << "Starting " << converter.GetEngineName() << "\n";
        auto documents = converter.GetTextDocuments();
        InvertedIndex index;
        index.UpdateDocumentBase(std::move(documents));
        SearchServer server(index);
        const auto requests = converter.GetRequests();
        const auto results = server.search(requests);
        std::vector<std::vector<std::pair<int, float>>> answers;
        answers.reserve(results.size());
        for (const auto& result : results) {
            std::vector<std::pair<int, float>> converted;
            for (const auto& entry : result) converted.emplace_back(static_cast<int>(entry.doc_id), entry.rank);
            answers.push_back(std::move(converted));
        }
        converter.putAnswers(answers);
        std::cout << "Indexed " << index.DocumentCount() << " document slots, processed " << requests.size()
                  << " requests. Results: " << workdir << "/answers.json\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << '\n';
        return 1;
    }
}
