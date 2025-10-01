#pragma once
#include "DataConstruct.h" 
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <fstream>


namespace gherkinexecutor {

    // Forward declarations
    class InputIterator;

    class TemplateConstruct;
    class StepConstruct;
    class ImportConstruct;
    class DefineConstruct;

    // Utility pair class template
    template<typename K, typename V>
    class Pair {
    private:
        K key;
        V value;

    public:
        Pair(const K& k, const V& v) : key(k), value(v) {}

        const K& getFirst() const { return key; }
        const V& getSecond() const { return value; }

        std::string toString() const {
            return "Pair{key=" + std::to_string(key) + ", value=" + std::to_string(value) + "}";
        }
    };

    class Translate {
    private:
        std::unordered_map<std::string, std::string> scenarios;
        std::unordered_map<std::string, std::string> glueFunctions;
        std::unordered_map<std::string, std::string> dataNames;
        std::unordered_map<std::string, std::string> dataNamesInternal;
        std::unordered_map<std::string, std::string> importNames;

        std::vector<std::string> linesToAddForDataAndGlue;
        std::vector<std::string> linesToAddToEndOfGlue;
        std::unordered_map<std::string, std::string> defineNames;

        int stepCount;
        std::string glueClass;
        std::string glueObject;

        int stepNumberInScenario;
        std::unique_ptr<InputIterator> dataIn;
        bool firstScenario;
        bool addBackground;
        bool addCleanup;
        bool inCleanup;
        bool finalCleanup;

        std::unique_ptr<std::ofstream> testFile;
        std::unique_ptr<std::ofstream> testFileHeader;
        bool featureActedOn;
        std::string featureName;
        std::string directoryName;
        std::string featureDirectory;
        std::string featurePackagePath;
        std::string packagePath;

        std::vector<std::string> classDataNames;

        std::unique_ptr<DataConstruct> dataConstruct;
        std::unique_ptr<TemplateConstruct> templateConstruct;
        std::unique_ptr<StepConstruct> stepConstruct;
        std::unique_ptr<ImportConstruct> importConstruct;
        std::unique_ptr<DefineConstruct> defineConstruct;

        std::string filterExpression;
        bool skipSteps;

        int scenarioCount;
        int backgroundCount;
        int cleanupCount;

        static const std::string TAG_INDICATOR;
        std::string tagLine;
        int tagLineNumber;

        bool errorOccurred;

    public:
        Translate();
        ~Translate();

        // Main public methods
        void translateInTests(const std::string& name);
        void writeInputFeature(const std::string& filename);

        // Utility methods accessible by other classes
        std::string quoteIt(const std::string& defaultVal);
        std::string fromImportData(const std::string& dataType, const std::string& value);
        std::string makeValueFromString(const DataConstruct::DataValues& variable, bool makeNameValue);
 
        // ... rest of the file remains unchanged ...
        std::string makeBuildName(const std::string& s);
        std::string makeName(const std::string& input);
        void trace(const std::string& value);
        void error(const std::string& value);
        void warning(const std::string& value);
        void testPrint(const std::string& line);
        void testPrintHeader(const std::string& line);
        std::vector<std::string> parseLine(const std::string& line);
        std::pair<std::string, std::vector<std::string>> lookForFollow();
        std::string replaceDefines(const std::string& in);

        // Friend classes need access to private members
        friend class DataConstruct;
        friend class TemplateConstruct;
        friend class StepConstruct;
        friend class ImportConstruct;
        friend class DefineConstruct;
        friend class InputIterator;

        // Static utility methods
        static std::string filterWord(const std::string& input);
        static std::vector<std::string> findFeatureFiles(const std::string& directory);
        static void readFilterList();
        static void readOptionList();
        static void readFeatureList();

        static void processArguments(const std::vector<std::string>& args);
        static std::vector<std::string> extractNamespaceParts(const std::string& packagePath);
    private:
        void alterFeatureDirectory();
        void findFeatureDirectory(const std::string& name);
        void actOnLine(const std::string& line, int pass);
        std::pair<std::vector<std::string>, std::vector<std::string>> splitLine(const std::string& line, int pass);
        void actOnKeyword(const std::string& keyword, const std::vector<std::string>& words,
            const std::vector<std::string>& comment, int pass);
        void checkForTagLine();
        void actOnFeature(const std::string& fullName);
        void setupnamespace();
        int processNamespaces(const std::string& packagePath);
        void endNamespace(const std::string& packagePath);
        bool actOnFeatureFirstHalf(const std::string& fullName);
        void actOnScenario(const std::string& fullName);
        void actOnBackground(const std::string& fullName);
        void actOnCleanup(const std::string& fullName);
        std::string logIt();
        void endUp();

        std::vector<std::string> readTable();
        std::vector<std::string> readString();
        int countIndent(const std::string& firstLine);

        static std::string makeFullName(const std::vector<std::string>& words);
        static std::string wordWithOutColon(const std::string& word);
        static std::string wordWithOutHash(const std::string& word);
        static void collectFeatureFiles(const std::string& dir, std::vector<std::string>& featureFiles);
public:
    static void printFlow(const std::string& value);
    };

} // namespace gherkinexecutor