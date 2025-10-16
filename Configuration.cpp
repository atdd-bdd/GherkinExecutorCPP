#include "Configuration.h"
#include <sstream>
#include <algorithm>
#include <stdexcept>

namespace gherkinexecutor {
    // Static member definitions
    bool Configuration::logIt = false;
    bool Configuration::inTest = false;
    bool Configuration::traceOn = false;
    char Configuration::spaceCharacters = ' ';
    bool Configuration::addLineToString = false;
    std::string Configuration::doNotCompare = "";
    std::string Configuration::currentDirectory = "";
    std::string Configuration::featureSubDirectory = "";
    std::string Configuration::treeDirectory = "";
    std::string Configuration::startingFeatureDirectory = "";
    bool Configuration::searchTree = false;
    std::string Configuration::packageName = "";
    std::string Configuration::testSubDirectory = "";
    std::string Configuration::dataDefinitionFileExtension = "";
    std::string Configuration::testFramework = "";
    std::string Configuration::addToPackageName = "";
    std::vector<std::string> Configuration::linesToAddForDataAndGlue = {};
    std::string Configuration::filterExpression = "";
    std::vector<std::string> Configuration::featureFiles = {};
    std::string Configuration::tagFilter = "";
    bool Configuration::oneDataFile = false;

    // Helper functions for YAML parsing and writing
    namespace {
        std::string trim(const std::string& str) {
            size_t first = str.find_first_not_of(' ');
            if (first == std::string::npos) return "";
            size_t last = str.find_last_not_of(' ');
            return str.substr(first, (last - first + 1));
        }

        std::string escapeYamlString(const std::string& str) {
            if (str.empty()) return "\"\"";

            // Check if string needs quoting
            bool needsQuotes = false;
            if (str.find(':') != std::string::npos ||
                str.find('[') != std::string::npos ||
                str.find(']') != std::string::npos ||
                str.find('#') != std::string::npos ||
                str.find('\n') != std::string::npos ||
                str.find('\t') != std::string::npos ||
                str.find('\'') != std::string::npos ||
                str.find('"') != std::string::npos ||
                str.front() == ' ' || str.back() == ' ') {
                needsQuotes = true;
            }

            if (!needsQuotes) {
                // Check for yaml reserved words
                std::string lower = str;
                std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
                if (lower == "true" || lower == "false" || lower == "null" ||
                    lower == "yes" || lower == "no" || lower == "on" || lower == "off") {
                    needsQuotes = true;
                }
            }

            if (needsQuotes) {
                std::string escaped = "\"";
                for (char c : str) {
                    if (c == '"') escaped += "\\\"";
                    else if (c == '\\') escaped += "\\\\";
                    else if (c == '\n') escaped += "\\n";
                    else if (c == '\t') escaped += "\\t";
                    else escaped += c;
                }
                escaped += "\"";
                return escaped;
            }
            return str;
        }

        std::string unescapeYamlString(std::string str) {
            str = trim(str);
            if (str.length() >= 2 && str.front() == '"' && str.back() == '"') {
                str = str.substr(1, str.length() - 2);
                std::string result;
                for (size_t i = 0; i < str.length(); ++i) {
                    if (str[i] == '\\' && i + 1 < str.length()) {
                        char next = str[i + 1];
                        if (next == '"') { result += '"'; i++; }
                        else if (next == '\\') { result += '\\'; i++; }
                        else if (next == 'n') { result += '\n'; i++; }
                        else if (next == 't') { result += '\t'; i++; }
                        else result += str[i];
                    }
                    else {
                        result += str[i];
                    }
                }
                return result;
            }
            return str;
        }

        std::vector<std::string> parseYamlArray(const std::string& value) {
            std::vector<std::string> result;
            std::string trimmed = trim(value);

            if (trimmed.empty() || trimmed == "[]") {
                return result;
            }

            // Remove brackets if present
            if (trimmed.front() == '[' && trimmed.back() == ']') {
                trimmed = trimmed.substr(1, trimmed.length() - 2);
            }

            std::stringstream ss(trimmed);
            std::string item;
            bool inQuotes = false;
            char quoteChar = '\0';
            std::string buffer;

            while (ss.good()) {
                char c = ss.get();
                if (ss.eof()) break;

                if (inQuotes) {
                    if (c == quoteChar) {
                        inQuotes = false;
                    }
                    else if (c == '\\') {
                        // Handle escape sequences
                        char next = ss.peek();
                        if (next == quoteChar || next == '\\') {
                            ss.get(); // consume escaped char
                            buffer += next;
                        }
                        else {
                            buffer += c;
                        }
                    }
                    else {
                        buffer += c;
                    }
                }
                else {
                    if (c == '\'' || c == '"') {
                        inQuotes = true;
                        quoteChar = c;
                    }
                    else if (c == ',') {
                        std::string cleanItem = trim(buffer);
                        if (!cleanItem.empty()) {
                            result.push_back(cleanItem);
                        }
                        buffer.clear();
                    }
                    else {
                        buffer += c;
                    }
                }
            }

            // Add last item
            std::string cleanItem = trim(buffer);
            if (!cleanItem.empty()) {
                result.push_back(cleanItem);
            }

            return result;
        }

        // Parse multi-line YAML array (lines starting with -)
        std::vector<std::string> parseMultiLineYamlArray(std::ifstream& file) {
            std::vector<std::string> result;
            std::streampos lastPos = file.tellg();
            std::string line;

            while (std::getline(file, line)) {
                std::string trimmedLine = trim(line);

                // Check if this line is an array item (starts with -)
                if (trimmedLine.empty() || trimmedLine[0] != '-') {
                    // Not an array item, rewind to before this line
                    file.seekg(lastPos);
                    break;
                }

                // Remove the leading '-' and any whitespace after it
                std::string item = trimmedLine.substr(1);
                item = trim(item);

                // Unescape the string value
                item = unescapeYamlString(item);

                if (!item.empty()) {
                    result.push_back(item);
                }

                lastPos = file.tellg();
            }

            return result;
        }

        std::string formatYamlArray(const std::vector<std::string>& vec) {
            if (vec.empty()) {
                return "[]";
            }

            std::ostringstream oss;
            oss << "[";
            for (size_t i = 0; i < vec.size(); ++i) {
                oss << escapeYamlString(vec[i]);
                if (i < vec.size() - 1) {
                    oss << ", ";
                }
            }
            oss << "]";
            return oss.str();
        }

        // Format array in multi-line YAML format
        void writeMultiLineYamlArray(std::ofstream& file, const std::vector<std::string>& vec) {
            if (vec.empty()) {
                file << " []\n";
                return;
            }

            file << "\n";
            for (const auto& item : vec) {
                file << "    - " << escapeYamlString(item) << "\n";
            }
        }

        bool parseBool(const std::string& value) {
            std::string lower = value;
            std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
            lower = trim(lower);
            return (lower == "true" || lower == "yes" || lower == "on" || lower == "1");
        }

        std::string formatBool(bool value) {
            return value ? "true" : "false";
        }

        char parseChar(const std::string& value) {
            std::string unescaped = unescapeYamlString(value);
            std::cout << "unescaped is '" << unescaped << "'" << std::endl;

            // Remove quotes (single or double)
            if (unescaped.length() >= 2) {
                char first = unescaped.front();
                char last = unescaped.back();
                if ((first == '\'' && last == '\'') ||
                    (first == '"' && last == '"')) {
                    unescaped = unescaped.substr(1, unescaped.length() - 2);
                    std::cout << "after removing quotes: '" << unescaped << "'" << std::endl;
                }
            }

            return unescaped.empty() ? ' ' : unescaped[0];
        }

        std::string formatChar(char c) {
            if (c == ' ') return "\" \"";
            if (c == '\t') return "\"\\t\"";
            if (c == '\n') return "\"\\n\"";
            return std::string(1, c);
        }
    }

    void Configuration::writeToYamlFile(const std::string& filename) {
        try {
            std::ofstream file(filename);
            if (!file.is_open()) {
                throw std::runtime_error("Cannot open file for writing: " + filename);
            }

            file << "# Gherkin Executor Configuration\n";
            file << "# Generated automatically - modify with care\n\n";

            // Boolean values
            file << "# Logging and debugging settings\n";
            file << "logIt: " << formatBool(logIt) << "\n";
            file << "inTest: " << formatBool(inTest) << "\n";
            file << "traceOn: " << formatBool(traceOn) << "\n\n";

            // Character and string formatting
            file << "# Character and string formatting\n";
            file << "spaceCharacters: " << formatChar(spaceCharacters) << "\n";
            file << "addLineToString: " << formatBool(addLineToString) << "\n";
            file << "doNotCompare: " << escapeYamlString(doNotCompare) << "\n\n";

            // Directory settings
            file << "# Directory settings\n";
            file << "currentDirectory: " << escapeYamlString(currentDirectory) << "\n";
            file << "featureSubDirectory: " << escapeYamlString(featureSubDirectory) << "\n";
            file << "treeDirectory: " << escapeYamlString(treeDirectory) << "\n";
            file << "startingFeatureDirectory: " << escapeYamlString(startingFeatureDirectory) << "\n";
            file << "searchTree: " << formatBool(searchTree) << "\n\n";

            // Package and test settings
            file << "# Package and test settings\n";
            file << "packageName: " << escapeYamlString(packageName) << "\n";
            file << "testSubDirectory: " << escapeYamlString(testSubDirectory) << "\n";
            file << "dataDefinitionFileExtension: " << escapeYamlString(dataDefinitionFileExtension) << "\n";
            file << "testFramework: " << escapeYamlString(testFramework) << "\n";
            file << "addToPackageName: " << escapeYamlString(addToPackageName) << "\n\n";

            // Arrays in multi-line format
            file << "# Additional imports/lines to add\n";
            file << "linesToAddForDataAndGlue:";
            writeMultiLineYamlArray(file, linesToAddForDataAndGlue);
            file << "\n";

            // Filtering and file selection
            file << "# Filtering and file selection\n";
            file << "filterExpression: " << escapeYamlString(filterExpression) << "\n";
            file << "tagFilter: " << escapeYamlString(tagFilter) << "\n";
            file << "oneDataFile: " << formatBool(oneDataFile) << "\n\n";

            file << "# Feature files to process\n";
            file << "featureFiles:";
            writeMultiLineYamlArray(file, featureFiles);

            file.close();

        }
        catch (const std::exception& e) {
            throw std::runtime_error("Error writing configuration: " + std::string(e.what()));
        }
    }

    void Configuration::readFromYamlFile(const std::string& filename) {
        try {
            if (!std::filesystem::exists(filename)) {
                throw std::runtime_error("Configuration file does not exist: " + filename);
            }

            std::ifstream file(filename);
            if (!file.is_open()) {
                throw std::runtime_error("Cannot open file for reading: " + filename);
            }

            std::string line;
            while (std::getline(file, line)) {
                // Skip comments and empty lines
                std::string trimmedLine = trim(line);
                if (trimmedLine.empty() || trimmedLine[0] == '#') {
                    continue;
                }

                // Find the colon separator
                size_t colonPos = line.find(':');
                if (colonPos == std::string::npos) {
                    continue; // Skip malformed lines
                }

                std::string key = trim(line.substr(0, colonPos));
                std::string value = trim(line.substr(colonPos + 1));

                // Parse based on key
                if (key == "logIt") {
                    logIt = parseBool(value);
                }
                else if (key == "inTest") {
                    inTest = parseBool(value);
                }
                else if (key == "traceOn") {
                    traceOn = parseBool(value);
                }
                else if (key == "spaceCharacters") {
                    spaceCharacters = parseChar(value);
                    std::cout << "value is '" << value << "'" << std::endl;
                    std::cout << "***** Space character set to '" << spaceCharacters << "'" << std::endl;
                }
                else if (key == "addLineToString") {
                    addLineToString = parseBool(value);
                }
                else if (key == "doNotCompare") {
                    doNotCompare = unescapeYamlString(value);
                }
                else if (key == "currentDirectory") {
                    currentDirectory = unescapeYamlString(value);
                }
                else if (key == "featureSubDirectory") {
                    featureSubDirectory = unescapeYamlString(value);
                }
                else if (key == "treeDirectory") {
                    treeDirectory = unescapeYamlString(value);
                }
                else if (key == "startingFeatureDirectory") {
                    startingFeatureDirectory = unescapeYamlString(value);
                }
                else if (key == "searchTree") {
                    searchTree = parseBool(value);
                }
                else if (key == "packageName") {
                    packageName = unescapeYamlString(value);
                }
                else if (key == "testSubDirectory") {
                    testSubDirectory = unescapeYamlString(value);
                }
                else if (key == "dataDefinitionFileExtension") {
                    dataDefinitionFileExtension = unescapeYamlString(value);
                }
                else if (key == "testFramework") {
                    testFramework = unescapeYamlString(value);
                }
                else if (key == "addToPackageName") {
                    addToPackageName = unescapeYamlString(value);
                }
                else if (key == "linesToAddForDataAndGlue") {
                    // Check if it's an inline array or multi-line array
                    if (value.empty() || value == "[]") {
                        // Multi-line array format - read next lines starting with -
                        linesToAddForDataAndGlue = parseMultiLineYamlArray(file);
                    }
                    else {
                        // Inline array format
                        linesToAddForDataAndGlue = parseYamlArray(value);
                    }
                }
                else if (key == "filterExpression") {
                    filterExpression = unescapeYamlString(value);
                }
                else if (key == "featureFiles") {
                    // Check if it's an inline array or multi-line array
                    if (value.empty() || value == "[]") {
                        // Multi-line array format
                        featureFiles = parseMultiLineYamlArray(file);
                    }
                    else {
                        // Inline array format
                        featureFiles = parseYamlArray(value);
                    }
                }
                else if (key == "tagFilter") {
                    tagFilter = unescapeYamlString(value);
                }
                else if (key == "oneDataFile") {
                    oneDataFile = parseBool(value);
                }
            }

            file.close();

        }
        catch (const std::exception& e) {
            throw std::runtime_error("Error reading configuration: " + std::string(e.what()));
        }
    }

    void Configuration::printAllConfiguration() {
        std::cout << "=== Gherkin Executor Configuration ===" << std::endl;
        std::cout << "logIt: " << (logIt ? "true" : "false") << std::endl;
        std::cout << "inTest: " << (inTest ? "true" : "false") << std::endl;
        std::cout << "traceOn: " << (traceOn ? "true" : "false") << std::endl;
        std::cout << "spaceCharacters: '" << spaceCharacters << "'" << std::endl;
        std::cout << "addLineToString: " << (addLineToString ? "true" : "false") << std::endl;
        std::cout << "doNotCompare: \"" << doNotCompare << "\"" << std::endl;
        std::cout << "currentDirectory: \"" << currentDirectory << "\"" << std::endl;
        std::cout << "featureSubDirectory: \"" << featureSubDirectory << "\"" << std::endl;
        std::cout << "treeDirectory: \"" << treeDirectory << "\"" << std::endl;
        std::cout << "startingFeatureDirectory: \"" << startingFeatureDirectory << "\"" << std::endl;
        std::cout << "searchTree: " << (searchTree ? "true" : "false") << std::endl;
        std::cout << "packageName: \"" << packageName << "\"" << std::endl;
        std::cout << "testSubDirectory: \"" << testSubDirectory << "\"" << std::endl;
        std::cout << "dataDefinitionFileExtension: \"" << dataDefinitionFileExtension << "\"" << std::endl;
        std::cout << "testFramework: \"" << testFramework << "\"" << std::endl;
        std::cout << "addToPackageName: \"" << addToPackageName << "\"" << std::endl;
        std::cout << "filterExpression: \"" << filterExpression << "\"" << std::endl;
        std::cout << "tagFilter: \"" << tagFilter << "\"" << std::endl;
        std::cout << "oneDataFile: " << (oneDataFile ? "true" : "false") << std::endl;

        std::cout << "linesToAddForDataAndGlue: [";
        for (size_t i = 0; i < linesToAddForDataAndGlue.size(); ++i) {
            std::cout << "\"" << linesToAddForDataAndGlue[i] << "\"";
            if (i < linesToAddForDataAndGlue.size() - 1) std::cout << ", ";
        }
        std::cout << "]" << std::endl;

        std::cout << "featureFiles: [";
        for (size_t i = 0; i < featureFiles.size(); ++i) {
            std::cout << "\"" << featureFiles[i] << "\"";
            if (i < featureFiles.size() - 1) std::cout << ", ";
        }
        std::cout << "]" << std::endl;
        std::cout << "====================================" << std::endl;
    }

    void Configuration::initialize() {
        // Set default values here if needed
        logIt = false;
        inTest = false;
        traceOn = false;
        spaceCharacters = ' ';
        addLineToString = false;
        doNotCompare = "";
        currentDirectory = std::filesystem::current_path().string();
        featureSubDirectory = "features";
        treeDirectory = "";
        startingFeatureDirectory = "";
        searchTree = false;
        packageName = "";
        testSubDirectory = "test";
        dataDefinitionFileExtension = ".json";
        testFramework = "gtest";
        addToPackageName = "";
        linesToAddForDataAndGlue.clear();
        filterExpression = "";
        featureFiles.clear();
        tagFilter = "";
        oneDataFile = false;
    }
}