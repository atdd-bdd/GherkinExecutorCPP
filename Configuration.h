#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <filesystem>



namespace gherkinexecutor {
    class Configuration {
    public:
        static bool logIt;
        static bool inTest;
        static bool traceOn;
        static char spaceCharacters;
        static bool addLineToString;
        static std::string doNotCompare;
        static std::string currentDirectory;
        static std::string featureSubDirectory;
        static std::string treeDirectory;
        static std::string startingFeatureDirectory;
        static bool searchTree;
        static std::string packageName;
        static std::string testSubDirectory;
        static std::string dataDefinitionFileExtension;
        static std::string testFramework;
        static std::string addToPackageName;
        static std::vector<std::string> linesToAddForDataAndGlue;
        static std::string filterExpression;
        static std::vector<std::string> featureFiles;
        static std::string tagFilter;
        static bool oneDataFile;

        static void initialize();

        // New YAML serialization methods
        static void writeToYamlFile(const std::string& filename);
        static void readFromYamlFile(const std::string& filename);
        // Debug/utility method
        static void printAllConfiguration();
    };
}
