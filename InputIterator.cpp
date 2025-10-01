#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include "InputIterator.h"
#include <iostream>
namespace gherkinexecutor {

    class Translate;
 
    const std::string InputIterator::EOF_STR = "EOF";

    InputIterator::InputIterator(const std::string& name, const std::string& featureDirectory, Translate* outer)
        : outer(outer), index(0), featureDirectory(featureDirectory) {
        if (!name.empty()) {
            readFile(name, 0);
        }
    }

    std::string InputIterator::toString() const {
        std::string temp;
        for (const std::string& line : linesIn) {
            temp += line + "\n";
        }
        return temp;
    }

    std::string InputIterator::peek() {
        if (index < static_cast<int>(linesIn.size())) {
            return linesIn[index];
        }
        return EOF_STR;
    }

    std::string InputIterator::next() {
        if (index < static_cast<int>(linesIn.size())) {
            return linesIn[index++];
        }
        return EOF_STR;
    }

    void InputIterator::readFile(const std::string& fileName, int includeCount) {
        includeCount++;
        if (includeCount > 20) {
            outer->error("Too many levels of include");
            return;
        }

        try {
            std::string filepath = Configuration::featureSubDirectory + fileName;
            Translate::printFlow("Path is " + filepath);

            std::ifstream file(filepath);
            if (!file.is_open()) {
                outer->error(" Unable to read " + filepath);
                return;
            }

            std::string line;
            while (std::getline(file, line)) {
                if (line.find("Include") == 0) {
                    // Parse include statement
                    std::vector<std::string> parts;
                    std::istringstream iss(line);
                    std::string word;
                    while (iss >> word) {
                        parts.push_back(word);
                    }

                    outer->trace("Parts are " + std::to_string(parts.size()));

                    bool localFile = true;
                    std::string includedFileName;

                    // Look for quoted filename
                    size_t start = line.find('"');
                    size_t end = line.find('"', start + 1);

                    if (start == std::string::npos) {
                        start = line.find('\'');
                        end = line.find('\'', start + 1);
                        localFile = false;
                    }

                    if (start == std::string::npos || end == std::string::npos) {
                        outer->error("Error filename not surrounded by quotes: " + line);
                        continue;
                    }

                    includedFileName = line.substr(start + 1, end - start - 1);

                    if (includedFileName.empty()) {
                        outer->error("Error zero length filename " + line);
                        continue;
                    }

                    // Trim whitespace
                    includedFileName.erase(0, includedFileName.find_first_not_of(" \t"));
                    includedFileName.erase(includedFileName.find_last_not_of(" \t") + 1);

                    if (localFile) {
                        includedFileName = featureDirectory + includedFileName;
                    }

                    outer->trace("Including " + includedFileName);

                    if (includedFileName.find(".csv") != std::string::npos) {
                        includeCSVFile(includedFileName);
                    }
                    else {
                        readFile(includedFileName, includeCount);
                    }
                }
                else {
                    if (!line.empty() && line[0] != '#') {
                        // Trim line
                        line.erase(0, line.find_first_not_of(" \t"));
                        line.erase(line.find_last_not_of(" \t") + 1);
                        linesIn.push_back(line);
                    }
                }
            }
            file.close();
        }
        catch (const std::exception& e) {
            std::cerr << e.what() << std::endl;
        }

        includeCount--;
    }

    void InputIterator::includeCSVFile(const std::string& includedFileName) {
        try {
            std::ifstream file(Configuration::featureSubDirectory + includedFileName);
            if (!file.is_open()) return;

            std::string line;
            while (std::getline(file, line)) {
                if (line.empty()) continue;

                std::string contents = convertCSVtoTable(line);
                // Trim contents
                contents.erase(0, contents.find_first_not_of(" \t"));
                contents.erase(contents.find_last_not_of(" \t") + 1);
                linesIn.push_back(contents);
            }
            file.close();
        }
        catch (const std::exception& e) {
            std::cerr << e.what() << std::endl;
        }
    }

    std::string InputIterator::convertCSVtoTable(const std::string& csvData) {
        std::vector<std::string> lines;
        std::istringstream stream(csvData);
        std::string line;

        while (std::getline(stream, line)) {
            std::vector<std::string> row = parseCsvLine(line);
            std::string formattedRow = "|";
            for (const std::string& cell : row) {
                formattedRow += cell + "|";
            }
            lines.push_back(formattedRow);
        }

        std::string result;
        for (size_t i = 0; i < lines.size(); ++i) {
            if (i > 0) result += "\n";
            result += lines[i];
        }

        return result;
    }

    std::vector<std::string> InputIterator::parseCsvLine(const std::string& line) {
        std::vector<std::string> result;
        std::string current;
        bool inQuotes = false;

        for (size_t i = 0; i < line.length(); ++i) {
            char c = line[i];

            if (c == '"') {
                if (inQuotes && i + 1 < line.length() && line[i + 1] == '"') {
                    current += '"';
                    ++i; // skip next quote
                }
                else {
                    inQuotes = !inQuotes;
                }
            }
            else if (c == ',' && !inQuotes) {
                result.push_back(current);
                current.clear();
            }
            else {
                current += c;
            }
        }

        result.push_back(current);
        return result;
    }

} // namespace gherkinexecutor