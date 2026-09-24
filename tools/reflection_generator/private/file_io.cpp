#include <file_io.h>

#include <fstream>

std::expected<void, std::string> writeTextFile(const std::filesystem::path& path, const std::string& content) {
    std::ofstream file(path);
    if (!file.is_open()) {
        return std::unexpected("Failed to open file: " + path.generic_string());
    }
    file << content;
    if (file.fail()) {
        return std::unexpected("failed to write into file: " + path.generic_string());
    }
    file.close();
    if (file.fail()) {
        return std::unexpected("failed to close file: " + path.generic_string());
    }

    return {};
}

std::expected<std::vector<std::string>, std::string> readTextLines(const std::filesystem::path& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return std::unexpected("Failed to open file: " + path.generic_string());
    }

    std::vector<std::string> result{};
    std::string currentLine;

    while (std::getline(file, currentLine)) {
        result.push_back(currentLine);
    }

    if (file.bad() || !file.eof()) {
        return std::unexpected("Failed to read file correctly: " + path.generic_string());
    }

    return result;
}
