#include <gtest/gtest.h>
#include "../src/include/parser.hpp"

TEST(CommandParserTest, SetPromptValidText) {
    std::string input = "/set_prompt --text \"You are a helpful assistant.\"";
    
    // Asumsi class Parser Anda mengembalikan struktur data Command
    Command cmd = CommandParser::parse(input);
    
    EXPECT_EQ(cmd.type, CommandType::SET_PROMPT);
    EXPECT_EQ(cmd.args["text"], "You are a helpful assistant.");
}

TEST(CommandParserTest, SetPromptEmptyText) {
    std::string input = "/set_prompt --text \"\"";
    Command cmd = CommandParser::parse(input);
    
    EXPECT_EQ(cmd.type, CommandType::SET_PROMPT);
    EXPECT_TRUE(cmd.args["text"].empty());
}

TEST(CommandParserTest, SetPromptMissingArgs) {
    std::string input = "/set_prompt";
    
    // Memastikan parser melemparkan exception atau mengembalikan status error
    EXPECT_THROW({
        CommandParser::parse(input);
    }, std::invalid_argument);
}