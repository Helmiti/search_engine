#include "InvertedIndex.h"
#include "SearchServer.h"
#include <gtest/gtest.h>

TEST(TestCaseInvertedIndex, Basic) {
    InvertedIndex index;
    index.UpdateDocumentBase({"milk sugar salt", "milk a milk b milk c milk d"});
    const std::vector<Entry> expected{{0, 1}, {1, 4}};
    EXPECT_EQ(index.GetWordCount("milk"), expected);
    EXPECT_TRUE(index.GetWordCount("none").empty());
}
TEST(TestCaseInvertedIndex, UpdateReplacesOldIndex) {
    InvertedIndex index;
    index.UpdateDocumentBase({"milk"});
    index.UpdateDocumentBase({"water water"});
    EXPECT_TRUE(index.GetWordCount("milk").empty());
    EXPECT_EQ(index.GetWordCount("water"), (std::vector<Entry>{{0,2}}));
}
TEST(TestCaseSearchServer, TestSimple) {
    InvertedIndex index;
    index.UpdateDocumentBase({"milk milk milk milk water water water", "milk water water",
                              "milk milk milk milk milk water water water water water", "americano cappuccino"});
    SearchServer server(index);
    const auto result = server.search({"milk water", "sugar"});
    const std::vector<RelativeIndex> expected{{2, 1.0f}, {0, 0.7f}, {1, 0.3f}};
    EXPECT_EQ(result.at(0), expected);
    EXPECT_TRUE(result.at(1).empty());
}
TEST(TestCaseSearchServer, RequiresAllWords) {
    InvertedIndex index;
    index.UpdateDocumentBase({"cat dog", "cat", "dog", "cat dog dog"});
    SearchServer server(index);
    const auto result = server.search({"cat dog"});
    ASSERT_EQ(result[0].size(), 2U);
    EXPECT_EQ(result[0][0].doc_id, 3U);
    EXPECT_EQ(result[0][1].doc_id, 0U);
}
