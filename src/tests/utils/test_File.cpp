#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>

#include "utils/File.h"

namespace fs = std::filesystem;
using namespace utils::file;

class FileHelperTest : public ::testing::Test {
   protected:
    void SetUp() override {
        testDir = fs::temp_directory_path() / "file_utils_test";
        fs::create_directories(testDir);
    }

    void TearDown() override { fs::remove_all(testDir); }

    fs::path testDir;
};

TEST_F(FileHelperTest, SaveValidFile) {
    fs::path filePath = testDir / "test.txt";
    SaveFileStatus status = saveFile("Hello, world!", filePath);

    EXPECT_EQ(status, SaveFileStatus::Ok);
    EXPECT_TRUE(fs::exists(filePath));

    // Check content
    std::ifstream in(filePath);
    std::string content;
    std::getline(in, content);
    EXPECT_EQ(content, "Hello, world!");
}

TEST_F(FileHelperTest, MissingFilename) {
    fs::path filePath = testDir / "";  // empty filename
    SaveFileStatus status = saveFile("data", filePath);
    EXPECT_EQ(status, SaveFileStatus::MissingFilename);
}

TEST_F(FileHelperTest, MissingExtension) {
    fs::path filePath = testDir / "file";  // no extension
    SaveFileStatus status = saveFile("data", filePath);
    EXPECT_EQ(status, SaveFileStatus::MissingExtension);
}

TEST_F(FileHelperTest, InvalidDirectory) {
    // Create a file where a directory would be
    fs::path invalidDir = testDir / "not_a_dir";
    std::ofstream dummy(invalidDir);
    dummy.close();

    fs::path filePath = invalidDir / "file.txt";
    SaveFileStatus status = saveFile("data", filePath);
    EXPECT_EQ(status, SaveFileStatus::CannotCreateDirectory);
}