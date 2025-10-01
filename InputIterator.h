#pragma once
#include "Translate.h"
namespace gherkinexecutor {
    class Translate; 
    class InputIterator {
    private:
        Translate* outer;
        std::vector<std::string> linesIn;
        int index;
        std::string featureDirectory;

    public:
        static const std::string EOF_STR;

        InputIterator(const std::string& name, const std::string& featureDirectory, Translate* outer);

        int getLineNumber() const { return index; }
        std::string toString() const;
        std::string peek();
        std::string next();
        void goToEnd() { index = static_cast<int>(linesIn.size()); }
        bool isEmpty() const { return linesIn.empty(); }
        void reset() { index = 0; }

    private:
        void readFile(const std::string& fileName, int includeCount);
        void includeCSVFile(const std::string& includedFileName);
        std::string convertCSVtoTable(const std::string& csvData);
        std::vector<std::string> parseCsvLine(const std::string& line);
    };
}

