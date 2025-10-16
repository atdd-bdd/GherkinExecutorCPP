
// Change if needed to match compiler version 
#include <filesystem>
namespace fs = std::filesystem;

#include "Translate.h"
#include "Configuration.h"
#include "TagFilterEvaluator.h"
#include "InputIterator.h"
#include "DataConstruct.h"
#include "StepConstruct.h"
#include "TemplateConstruct.h"
#include "ImportConstruct.h"
#include "DefineConstruct.h"


#include <iostream>
#include <fstream>

#include <sstream>
#include <algorithm>
#include <cctype>
#include <regex>



namespace gherkinexecutor {

    // Static member initializations
    const std::string Translate::TAG_INDICATOR = "@";

    // Translate implementation
    Translate::Translate()
        : stepCount(0), stepNumberInScenario(0), firstScenario(true),
        addBackground(false), addCleanup(false), inCleanup(false),
        finalCleanup(false), featureActedOn(false), packagePath("Not Set"),
        skipSteps(false), scenarioCount(0), backgroundCount(0), cleanupCount(0),
        tagLineNumber(0), errorOccurred(false) {

        filterExpression = Configuration::filterExpression;
        //linesToAddForDataAndGlue = Configuration::linesToAddForDataAndGlue;

        dataConstruct = std::make_unique<DataConstruct>(this);
        templateConstruct = std::make_unique<TemplateConstruct>(this);
        stepConstruct = std::make_unique<StepConstruct>(this);
        importConstruct = std::make_unique<ImportConstruct>(this);
        defineConstruct = std::make_unique<DefineConstruct>(this);
    }

    Translate::~Translate() = default;

    void Translate::translateInTests(const std::string& name) {
        findFeatureDirectory(name);

        linesToAddForDataAndGlue.insert(linesToAddForDataAndGlue.end(),
            Configuration::linesToAddForDataAndGlue.begin(),
            Configuration::linesToAddForDataAndGlue.end());

        dataIn = std::make_unique<InputIterator>(name, featureDirectory, this);
        alterFeatureDirectory();

        if (dataIn->isEmpty())
            return;

        for (int pass = 1; pass <= 3; pass++) {
            dataIn->reset();
            bool eof = false;
            while (!eof) {
                std::string line = dataIn->next();
                if (line == InputIterator::EOF_STR) {
                    eof = true;
                    continue;
                }
                actOnLine(line, pass);
            }
        }
        endUp();
    }

    void Translate::findFeatureDirectory(const std::string& name) {
        std::string directory = "";
        size_t indexForward = name.find_last_of('/');
        size_t indexBack = name.find_last_of('\\');
        size_t index = std::max(indexForward, indexBack);

        if (index != std::string::npos) {
            directory = name.substr(0, index + 1);
        }

        featureDirectory = directory;
        featurePackagePath = featureDirectory;
        std::replace(featurePackagePath.begin(), featurePackagePath.end(), '\\', '.');
        std::replace(featurePackagePath.begin(), featurePackagePath.end(), '/', '.');
    }

    void Translate::alterFeatureDirectory() {
        std::string searchFor = Configuration::treeDirectory;
        std::string alternateSearchFor = searchFor;
        std::replace(alternateSearchFor.begin(), alternateSearchFor.end(), '/', '\\');

        std::string directory = featureDirectory;
        size_t pos = directory.find(searchFor);
        if (pos != std::string::npos) {
            directory.replace(pos, searchFor.length(), "");
        }
        pos = directory.find(alternateSearchFor);
        if (pos != std::string::npos) {
            directory.replace(pos, alternateSearchFor.length(), "");
        }

        featureDirectory = directory;
        featurePackagePath = featureDirectory;
        std::replace(featurePackagePath.begin(), featurePackagePath.end(), '\\', '.');
        std::replace(featurePackagePath.begin(), featurePackagePath.end(), '/', '.');
    }

    std::string Translate::filterWord(const std::string& input) {
        if (input.empty()) {
            return "";
        }

        std::string result;
        for (char c : input) {
            if (std::isalnum(c) || c == '_' || c == '*') {
                result += c;
            }
        }
        return result;
    }

    std::string Translate::wordWithOutColon(const std::string& word) {
        std::string result = word;
        // Remove leading and trailing colons
        while (!result.empty() && result.front() == ':') {
            result.erase(0, 1);
        }
        while (!result.empty() && result.back() == ':') {
            result.pop_back();
        }
        return result;
    }

    std::string Translate::wordWithOutHash(const std::string& word) {
        std::string result = word;
        // Remove leading and trailing hashes
        while (!result.empty() && result.front() == '#') {
            result.erase(0, 1);
        }
        while (!result.empty() && result.back() == '#') {
            result.pop_back();
        }
        return result;
    }

    std::pair<std::vector<std::string>, std::vector<std::string>> Translate::splitLine(const std::string& line, int pass) {
        std::istringstream iss(line);
        std::vector<std::string> allWords;
        std::string word;

        while (iss >> word) {
            allWords.push_back(word);
        }

        std::vector<std::string> words;
        std::vector<std::string> comment;

        if ((pass == 3 || pass == 2) && line.find(TAG_INDICATOR) == 0) {
            tagLine = line;
            tagLineNumber = dataIn->getLineNumber();
        }

        bool inComment = false;
        for (const std::string& aWord : allWords) {
            std::string trimmedWord = aWord;
            // Trim whitespace
            trimmedWord.erase(0, trimmedWord.find_first_not_of(" \t"));
            trimmedWord.erase(trimmedWord.find_last_not_of(" \t") + 1);

            if (trimmedWord.empty()) continue;

            if (!trimmedWord.empty() && trimmedWord.back() == ':') {
                trimmedWord = wordWithOutColon(trimmedWord);
            }

            if (trimmedWord.empty()) continue;

            if (inComment) {
                comment.push_back(trimmedWord);
                continue;
            }

            if (!trimmedWord.empty() && trimmedWord[0] == '#') {
                inComment = true;
                trimmedWord = wordWithOutHash(trimmedWord);
                if (!trimmedWord.empty()) {
                    comment.push_back(trimmedWord);
                }
                continue;
            }

            trimmedWord = filterWord(trimmedWord);
            words.push_back(trimmedWord);
        }

        return std::pair<std::vector<std::string>, std::vector<std::string>>(words, comment);
    }

    void Translate::actOnLine(const std::string& line, int pass) {
        auto splitResult = splitLine(line, pass);
        std::vector<std::string> words = splitResult.first;
        std::vector<std::string> comment = splitResult.second;

        if (!words.empty()) {
            std::string keyword = wordWithOutColon(words[0]);
            if (keyword == "*") {
                keyword = "Star";
            }
            words[0] = keyword;
            actOnKeyword(keyword, words, comment, pass);
        }
    }

    std::string Translate::makeFullName(const std::vector<std::string>& words) {
        std::string temp;
        for (size_t i = 0; i < words.size(); ++i) {
            if (i > 0) temp += "_";
            temp += words[i];
        }

        std::regex pattern("[^A-Za-z0-9_]");
        return std::regex_replace(temp, pattern, "_");
    }

    void Translate::actOnKeyword(const std::string& keyword, const std::vector<std::string>& words,
        const std::vector<std::string>& comment, int pass) {
        std::string fullName = makeFullName(words);
        trace("Act on keyword " + keyword + " " + fullName + " pass " + std::to_string(pass));

        std::string modifiedKeyword = keyword;
        std::vector<std::string> modifiedWords = words;

        if (keyword == "Star" && words.size() > 1) {
            if (words[1] == "Data") {
                modifiedKeyword = "Data";
                modifiedWords.erase(modifiedWords.begin());
            }
            else if (words[1] == "Import") {
                modifiedKeyword = "Import";
                modifiedWords.erase(modifiedWords.begin());
            }
            else if (words[1] == "Define") {
                modifiedKeyword = "Define";
                modifiedWords.erase(modifiedWords.begin());
            }
            else if (words[1] == "Cleanup") {
                modifiedKeyword = "Cleanup";
                modifiedWords.erase(modifiedWords.begin());
            }
        }

        if (modifiedKeyword == "Feature") {
            if (pass != 2) return;
            actOnFeature(fullName);
            if (TagFilterEvaluator::shouldNotExecute(comment, filterExpression)) {
                dataIn->goToEnd();
                std::cout << " Skip Entire Feature " << std::endl;
            }
        }
        else if (modifiedKeyword == "Scenario") {
            if (pass != 3) return;
            if (TagFilterEvaluator::shouldNotExecute(comment, filterExpression)) {
                skipSteps = true;
                return;
            }
            skipSteps = false;
            actOnScenario(fullName);
            inCleanup = false;
        }
        else if (modifiedKeyword == "Background") {
            addBackground = true;
            if (pass != 3) return;
            skipSteps = false;
            actOnBackground(fullName);
            inCleanup = true;
        }
        else if (modifiedKeyword == "Cleanup") {
            addCleanup = true;
            if (pass != 3) return;
            skipSteps = false;
            actOnCleanup(fullName);
            inCleanup = true;
        }
        else if (modifiedKeyword == "But" || modifiedKeyword == "Given" || modifiedKeyword == "When" ||
            modifiedKeyword == "Then" || modifiedKeyword == "And" || modifiedKeyword == "Star" ||
            modifiedKeyword == "Arrange" || modifiedKeyword == "Act" || modifiedKeyword == "Assert" ||
            modifiedKeyword == "Rule" || modifiedKeyword == "Calculation" || modifiedKeyword == "Trigger" ||
            modifiedKeyword == "Verify" || modifiedKeyword == "Assemble" || modifiedKeyword == "Activate" ||
            modifiedKeyword == "Preconditions" || modifiedKeyword == "MainCourse" ||
            modifiedKeyword == "Exception" || modifiedKeyword == "Postconditions") {
            if (pass != 3) return;
            if (skipSteps) return;
            stepConstruct->actOnStep(fullName, comment);
        }
        else if (modifiedKeyword == "Data") {
            if (pass != 2) return;
            skipSteps = false;
            dataConstruct->actOnData(modifiedWords);
        }
        else if (modifiedKeyword == "Import") {
            if (pass != 1) return;
            skipSteps = false;
            importConstruct->actOnImport(modifiedWords);
        }
        else if (modifiedKeyword == "Define") {
            if (pass != 1) return;
            skipSteps = false;
            defineConstruct->actOnDefine(modifiedWords);
        }
        // else: line not used this pass or bad line
    }

    void Translate::checkForTagLine() {
        if (tagLine.empty()) return;

        if (tagLineNumber + 1 == dataIn->getLineNumber()) {
            testPrint(tagLine);
        }
        tagLine = "";
        tagLineNumber = 0;
    }

    void Translate::writeInputFeature(const std::string& filename) {
        std::string fullFilename = filename + "feature.txt";
        printFlow("Logging to " + fullFilename);

        try {
            std::ofstream myLog(fullFilename);
            myLog << dataIn->toString();
            myLog.close();
        }
        catch (const std::exception& e) {
            std::cerr << e.what() << std::endl;
            std::cerr << "**** Cannot write to " << fullFilename << std::endl;
        }
    }

    void Translate::actOnFeature(const std::string& fullName) {
        if (actOnFeatureFirstHalf(fullName)) return;
        dataHeaderPrint("#pragma once");
        testPrintHeader("#pragma once");
        if (Configuration::testFramework == "gtest") {
            testPrint("#include <gtest/gtest.h>");
        }
        //   Change for other test frameworks if needed
        else if (Configuration::testFramework == "TestNG") {
            testPrint("import org.testng.annotations.*;");
        }
        else {
            testPrint("#include <gtest/gtest.h>");
        }

        testPrint("#include <vector>");
        testPrint("#include <string>");
        if (Configuration::logIt) {
            testPrint("#include <fstream>");
            testPrint("#include <iostream>");
            testPrint("#include <filesystem>");
        }
        testPrint("#include \"" + dataHeaderFilename + "\"");
        testPrint("#include \"" + glueClass + ".h\"");

        testPrint("using namespace gherkinexecutor::" + featureName + ";");
        setupnamespace();

        checkForTagLine();
        
        testPrint("class " + fullName + " : public ::testing::Test{");
        testPrint(logIt());
        testPrint("};");
        endNamespace(packagePath);
        templateConstruct->beginTemplate();
    }

    void Translate::setupnamespace()
    {
        processNamespaces(packagePath); 
    }
    int Translate::processNamespaces(const std::string& packagePath) {
        std::vector<std::string> parts = extractNamespaceParts(packagePath);

        for (const auto& part : parts) {
            testPrint("namespace " + part + " {");
        }

        return static_cast<int>(parts.size());
    }
    std::vector<std::string> Translate::extractNamespaceParts(const std::string& packagePath) {
        std::vector<std::string> parts;
        size_t start = 0;

        while (true) {
            size_t pos = packagePath.find('.', start);
            std::string part = packagePath.substr(start, pos - start);
            parts.push_back(part);

            if (pos == std::string::npos)
                break;

            start = pos + 1;
        }

        return parts;
    }
    void Translate::endNamespace(const std::string& packagePath) {
        size_t count = std::count(packagePath.begin(), packagePath.end(), '.');

        for (size_t i = 0; i < count + 1; ++i) {
            testPrint("}");
        }
    }
    bool Translate::actOnFeatureFirstHalf(const std::string& fullName) {
        if (featureActedOn) {
            warning("Feature keyword duplicated - it is ignored " + fullName);
            return true;
        }

        featureName = fullName;
        featureActedOn = true;
        packagePath = Configuration::addToPackageName + Configuration::packageName + "." + featurePackagePath + featureName;
        printFlow("Package is " + packagePath);

        std::string testPathname = Configuration::testSubDirectory + featureDirectory + featureName + "/" +
            featureName + ".cpp";
        std::string testPathnameHeader = Configuration::testSubDirectory + featureDirectory + featureName + "/" +
            featureName + ".h";
        dataHeaderFilename = 
            featureName + "_data.h";
        std::string dataHeaderPathname = Configuration::testSubDirectory + featureDirectory + featureName + "/" + dataHeaderFilename;
        printFlow(" Writing " + testPathname);


        std::string templateFilename = Configuration::testSubDirectory + featureDirectory + featureName + "/" +
            featureName + "_glue.cpp.tmpl";
        std::string templateFilenameHeader = Configuration::testSubDirectory + featureDirectory + featureName + "/" +
            featureName + "_glue.h.tmpl";

        directoryName = Configuration::testSubDirectory + featureDirectory + featureName;
        printFlow("Directory " + directoryName + " ");

        try {
            fs::create_directories(directoryName);

            testFile = std::make_unique<std::ofstream>(testPathname);
            testFileHeader = std::make_unique<std::ofstream>(testPathnameHeader);
            templateConstruct->glueTemplateFile = std::make_unique<std::ofstream>(templateFilename);
            templateConstruct->glueTemplateFileHeader = std::make_unique<std::ofstream>(templateFilenameHeader);
            dataHeaderFile = std::make_unique<std::ofstream>(dataHeaderPathname);
        }
        catch (const std::exception& e) {
            error("IO Exception in setting up the files");
            error(" Writing " + testPathname);
        }

        glueClass = fullName + "_glue";
        glueObject = makeName(fullName) + "_glue_object";
        writeInputFeature(Configuration::testSubDirectory + featureDirectory + featureName + "/");

        return false;
    }

    std::string Translate::makeBuildName(const std::string& s) {
        std::string temp = makeName(s);
        if (!temp.empty()) {
            temp[0] = std::toupper(temp[0]);
        }
        return "set" + temp;
    }

    std::string Translate::makeName(const std::string& input) {
        if (input.empty()) return "NAME_IS_EMPTY";

        std::string temp = input;
        std::replace(temp.begin(), temp.end(), ' ', '_');
        temp = filterWord(temp);

        if (!temp.empty()) {
            temp[0] = std::tolower(temp[0]);
        }
        return temp;
    }

    void Translate::actOnScenario(const std::string& fullName) {
        scenarioCount++;
        std::string fullNameToUse = fullName;

        if (scenarios.find(fullName) != scenarios.end()) {
            fullNameToUse += std::to_string(stepCount);
            error("Scenario name duplicated renamed " + fullNameToUse);
        }
        else {
            scenarios[fullNameToUse] = "";
        }

        stepNumberInScenario = 0;
        finalCleanup = addCleanup;

        if (firstScenario) {
            firstScenario = false;
        }
        else {
            if (addCleanup && !inCleanup) {
                testPrint("         test_Cleanup(" + glueObject + "); // from previous");
            }
            testPrint("        }"); // end previous scenario
        }

        checkForTagLine();

        testPrint("    TEST_F(" + featureName + "," + fullNameToUse + ") {");
        testPrint("         " + glueClass + " " + glueObject + ";");

        if (Configuration::logIt) {
            testPrint("        log(\"" + fullNameToUse + "\");");
        }

        if (addBackground) {
            testPrint("        test_Background(" + glueObject + ");");
        }
    }

    void Translate::actOnBackground(const std::string& fullName) {
        backgroundCount++;
        std::string fullNameToUse = fullName;
        finalCleanup = false;

        if (backgroundCount > 1) {
            error("More than one Background statement");
            fullNameToUse += std::to_string(backgroundCount);
        }

        stepNumberInScenario = 0;

        if (firstScenario) {
            firstScenario = false;
        }
        else {
            testPrint("        }"); // end previous scenario
        }

        testPrint("    static void test_" + fullNameToUse + "(" + glueClass + " & " + glueObject + "){");
        if (Configuration::logIt) {
            testPrint("        " + featureName +"::log(\"" + fullNameToUse + "\");");
        }
    }

    void Translate::actOnCleanup(const std::string& fullName) {
        cleanupCount++;
        finalCleanup = false;
        std::string fullNameToUse = fullName;

        if (cleanupCount > 1) {
            error("More than one cleanup statement");
            fullNameToUse += std::to_string(cleanupCount);
        }

        stepNumberInScenario = 0;

        if (firstScenario) {
            firstScenario = false;
        }
        else {
            testPrint("        }"); // end previous scenario
        }

        testPrint("    static void test_" + fullNameToUse + "(" + glueClass + " & " + glueObject + "){");
        if (Configuration::logIt) {
            testPrint("        " + featureName + "::log(\"" + fullNameToUse + "\");");
        }

    }
    std::string Translate::logIt() {
        if (Configuration::logIt) {
            std::string filename = directoryName + "/log.txt";
            
            return "public:  static void log(const std::string& value) {\n" +
                std::string(
                    "try {\n"
                    "  std::ofstream logFile(\"" + filename + "\", std::ios::app); // append mode\n"
                    "  if (logFile.is_open()) {\n"
                    "    logFile << value << \"\\n\";\n"
                    "    logFile.close();\n"
                    "  }\n"
                    "  else {\n"
                    "    std::cerr << \"*** Cannot write to log\" << std::endl;\n"
                    "  }\n"
                    "}\n"
                    "catch (const std::exception& e) {\n"
                    "std::cerr << \" * **Cannot write to log : \" << e.what() << std::endl;\n"
                    "}\n"
                    " }\n");
        }
        else {
            return "";
        }
    }

    void Translate::trace(const std::string& value) {
        if (Configuration::traceOn) {
            std::cout << value << std::endl;
        }
    }

    void Translate::error(const std::string& value) {
        if (dataIn != nullptr)
            std::cerr << "[GherkinExecutor] ~ line " << dataIn->getLineNumber()
            << " in feature.txt " << value << " " << std::endl;
        else 
            std::cerr << "[GherkinExecutor] " << " in feature.txt " << value << " " << std::endl;

        errorOccurred = true;
    }

    void Translate::warning(const std::string& value) {
        std::cerr << "[GherkinExecutor] Warning ~ line " << dataIn->getLineNumber()
            << " in feature.txt " << value << std::endl;
    }

    void Translate::printFlow(const std::string& value) {
        std::cout << value << std::endl;
    }

    void Translate::testPrint(const std::string& line) {
        try {
            *testFile << line << std::endl;
        }
        catch (const std::exception& e) {
            error("IO ERROR ");
        }
    }
    void Translate::dataHeaderPrint(const std::string& line) {
        try {
            *dataHeaderFile << line << std::endl;
        }
        catch (const std::exception& e) {
            error("IO ERROR ");
        }
    }
    void Translate::testPrintHeader(const std::string& line) {
        try {
            *testFileHeader << line << std::endl;
        }
        catch (const std::exception& e) {
            error("IO ERROR ");
        }
    }

    std::vector<std::string> Translate::parseLine(const std::string& line) {
        std::string lineShort = line;
        // Trim whitespace
        lineShort.erase(0, lineShort.find_first_not_of(" \t"));
        lineShort.erase(lineShort.find_last_not_of(" \t") + 1);

        if (lineShort.empty() || lineShort[0] != '|') {
            error("table not begin with | " + line);
            return { "ERROR IN TABLE LINE " + line };
        }

        lineShort = lineShort.substr(1); // remove first |

        if (lineShort.empty()) {
            error("table format error " + line);
            return { "ERROR IN TABLE LINE " + line };
        }

        if (lineShort.back() == '|') {
            lineShort.pop_back(); // remove last |
        }
        else {
            error("table not end with | " + line);
        }

        std::vector<std::string> elements;
        std::istringstream iss(lineShort);
        std::string element;

        while (std::getline(iss, element, '|')) {
            // Trim element
            element.erase(0, element.find_first_not_of(" \t"));
            element.erase(element.find_last_not_of(" \t") + 1);

            std::replace(element.begin(), element.end(), Configuration::spaceCharacters, ' ');
            element = replaceDefines(element);
            elements.push_back(element);
        }

        return elements;
    }

   std::pair<std::string, std::vector<std::string>> Translate::lookForFollow() {
        std::string line = dataIn->peek();
        std::vector<std::string> empty;

        while (!line.empty() && line[0] == '#') {
            dataIn->next();
            line = dataIn->peek();
        }

        // Trim line
        line.erase(0, line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t") + 1);

        if (line.empty()) return std::pair<std::string, std::vector<std::string>>("NOTHING", empty);

        if (line[0] == '|') {
            std::vector<std::string> retValue = readTable();
            trace("Table is " + std::to_string(retValue.size()) + " lines");
            return std::pair<std::string, std::vector<std::string>>("TABLE", retValue);
        }

        if (line == "\"\"\"") {
            std::vector<std::string> retValue = readString();
            trace("Multi Line String is " + std::to_string(retValue.size()) + " lines");
            return std::pair<std::string, std::vector<std::string>>("STRING", retValue);
        }

        return std::pair<std::string, std::vector<std::string>>("NOTHING", empty);
    }

    std::vector<std::string> Translate::readTable() {
        std::vector<std::string> retValue;
        std::string line = dataIn->peek();

        // Trim line
        line.erase(0, line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t") + 1);

        // Remove comments
        size_t commentPos = line.find('#');
        if (commentPos != std::string::npos) {
            line = line.substr(0, commentPos);
            line.erase(line.find_last_not_of(" \t") + 1);
        }

        while (!line.empty() && (line[0] == '|' || line[0] == '#')) {
            line = dataIn->next();
            line.erase(0, line.find_first_not_of(" \t"));
            line.erase(line.find_last_not_of(" \t") + 1);

            commentPos = line.find('#');
            if (commentPos != std::string::npos) {
                line = line.substr(0, commentPos);
                line.erase(line.find_last_not_of(" \t") + 1);
            }

            if (!line.empty() && line[0] == '|' && line.back() == '|') {
                retValue.push_back(line);
            }
            else if (!line.empty() && line[0] == '|') {
                error("Invalid line in table " + line);
            }

            line = dataIn->peek();
            line.erase(0, line.find_first_not_of(" \t"));
            line.erase(line.find_last_not_of(" \t") + 1);
        }

        return retValue;
    }

    std::vector<std::string> Translate::readString() {
        std::vector<std::string> retValue;
        std::string firstLine = dataIn->peek();
        int countIndentVal = countIndent(firstLine);

        dataIn->next(); // skip the """
        std::string line = dataIn->next();

        while (line.find("\"\"\"") == std::string::npos) {
            if (static_cast<int>(line.length()) > countIndentVal) {
                retValue.push_back(line.substr(countIndentVal));
            }
            else {
                retValue.push_back("");
            }
            line = dataIn->next();
        }

        return retValue;
    }

    std::string Translate::replaceDefines(const std::string& in) {
        bool didReplacement = true;
        std::string changeString = in;

        while (didReplacement) {
            didReplacement = false;
            for (const auto& pair : defineNames) {
                const std::string& name = pair.first;
                const std::string& replacement = pair.second;

                size_t pos = changeString.find(name);
                if (pos != std::string::npos) {
                    didReplacement = true;
                    changeString.replace(pos, name.length(), replacement);
                }
            }
        }

        return changeString;
    }

    int Translate::countIndent(const std::string& firstLine) {
        std::string line = firstLine;
        line.erase(0, line.find_first_not_of(" \t"));
        return static_cast<int>(firstLine.length() - line.length());
    }

    std::string Translate::quoteIt(const std::string& defaultVal) {
        return "\"" + defaultVal + "\"";
    }

    std::string Translate::fromImportData(const std::string& dataType, const std::string& value) {
        auto it = importNames.find(dataType);
        if (it != importNames.end()) {
            std::string conversionMethod = it->second;
            size_t pos = conversionMethod.find("$");
            if (pos != std::string::npos) {
                conversionMethod.replace(pos, 1, value);
            }
            return conversionMethod;
        }
        return "";
    }

    std::string Translate::makeValueFromString(const DataConstruct::DataValues& variable, bool makeNameValue) {
        std::string value;
        if (makeNameValue) {
            value = makeName(variable.name);
        }
        else {
            value = quoteIt(variable.defaultVal);
        }

        if (variable.dataType == "std::string") {
            return value;
        }
        else if (variable.dataType == "int") {
            return "std::stoi(" + value + ")";
        }
        else if (variable.dataType == "double") {
            return "std::stod(" + value + ")";
        }
        else if (variable.dataType == "signed char") {
            return "static_cast<signed char>((" + value + ".length() > 0 ? " + value + "[0] : ' '))";
         }
        else if (variable.dataType == "unsigned char") {
            return "static_cast<unsigned char>((" + value + ".length() > 0 ? " + value + "[0] : ' '))";
        }
        else if (variable.dataType == "short") {
            return "static_cast<short>(std::stoi(" + value + "))";
        }
        else if (variable.dataType == "unsigned short") {
            return "static_cast<unsigned short>(std::stoul(" + value + "))";
        }
        else if (variable.dataType == "long") {
            return "std::stol(" + value + ")";
        }
        else if (variable.dataType == "unsigned long") {
            return "std::stoul(" + value + ")";
        }
        else if (variable.dataType == "long long") {
            return "std::stoll(" + value + ")";
        }
        else if (variable.dataType == "unsigned long long") {
            return "std::stoull(" + value + ")";
        }
        else if (variable.dataType == "float") {
            return "std::stof(" + value + ")";
        }
        else if (variable.dataType == "long double") {
            return "static_cast<long double>(std::stold(" + value + "))";
        }
        else if (variable.dataType == "bool") {
            return "(" + value + " == \"true\" || " + value + " == \"1\")";
        }
        else if (variable.dataType == "char") {
            return "(" + value + ".length() > 0 ? " + value + "[0] : ' ')";
        }
        else if (variable.dataType == "unsigned int") {
            return "static_cast<unsigned int>(std::stoul(" + value + "))";
        }
        else {
            std::string result = fromImportData(variable.dataType, value);
            if (!result.empty()) {
                return result;
            }
            return "" + variable.dataType + "(" + value + ")";
        }
    }
    void Translate::endUp() {
        if (finalCleanup) {
            testPrint("         test_Cleanup(" + glueObject + "); // at the end");
        }

        if (scenarioCount == 0) {
            std::cout << "No scenarios" << std::endl;
        }
        else {
            testPrint("        }"); // End last scenario
        }

        testPrint("");

        try {
            if (testFile) {
                testFile->close();
            }
        }
        catch (const std::exception& e) {
            error("Error in closing ");
        }
        try {
            if (testFileHeader) {
                testFileHeader->close();
            }
        }
        catch (const std::exception& e) {
            error("Error in closing ");
        }
        try {
            if (dataHeaderFile) {
                dataHeaderFile->close();
            }
        }
        catch (const std::exception& e) {
            error("Error in closing ");
        }

        templateConstruct->endTemplate();

        if (errorOccurred) {
            std::cerr << "*** Error in translation, scan the output" << std::endl;
            std::exit(-1);
        }

        dataConstruct->endOneDataFile();
    }

    // Static methods
    std::vector<std::string> Translate::findFeatureFiles(const std::string& directory) {
        std::vector<std::string> featureFiles;
        collectFeatureFiles(directory, featureFiles);
        return featureFiles;
    }

    void Translate::collectFeatureFiles(const std::string& dir, std::vector<std::string>& featureFiles) {
        std::string remove = Configuration::featureSubDirectory;
        std::replace(remove.begin(), remove.end(), '/', '\\');

        try {
            for (const auto& entry : fs::recursive_directory_iterator(dir)) {
                if (entry.is_regular_file() && entry.path().extension() == ".featurex") {
                    std::string path = entry.path().string();
                    size_t pos = path.find(remove);
                    if (pos != std::string::npos) {
                        path.replace(pos, remove.length(), "");
                    }
                    featureFiles.push_back(path);
                }
            }
        }
        catch (const std::exception& e) {
            std::cerr << "Error reading directory: " << e.what() << std::endl;
        }
    }

    void Translate::readFilterList() {
        std::string filepath = Configuration::featureSubDirectory + "filter.txt";
        printFlow("Path is " + filepath);

        try {
            std::ifstream file(filepath);
            if (file.is_open()) {
                std::string line;
                if (std::getline(file, line)) {
                    Configuration::filterExpression = line;
                    std::cout << "Filter is " << line << std::endl;
                }
                file.close();
            }
        }
        catch (const std::exception& e) {
            // File not found or other error - ignore silently
        }
    }

    void Translate::readOptionList() {
        std::string filepath = Configuration::featureSubDirectory + "options.txt";
        printFlow("Path is " + filepath);

        try {
            std::ifstream file(filepath);
            if (file.is_open()) {
                std::vector<std::string> lines;
                std::string line;
                while (std::getline(file, line)) {
                    lines.push_back(line);
                }
                file.close();
                processArguments(lines);
            }
        }
        catch (const std::exception& e) {
            // File not found or other error - ignore silently
        }
    }

    void Translate::readFeatureList() {
        std::string filepath = Configuration::featureSubDirectory + "features.txt";
        printFlow("Path is " + filepath);

        try {
            std::ifstream file(filepath);
            if (file.is_open()) {
                std::string line;
                while (std::getline(file, line)) {
                    Configuration::featureFiles.push_back(line);
                }
                file.close();
            }
        }
        catch (const std::exception& e) {
            // File not found or other error - ignore silently
        }
    }

    void Translate::processArguments(const std::vector<std::string>& args) {
        printFlow("Optional arguments are logIt inTest searchTree traceOn");
        bool filterNext = false;

        for (const std::string& arg : args) {
            printFlow("Program argument: " + arg);

            if (arg == "logIt") {
                Configuration::logIt = true;
                printFlow("logIt on");
            }
            else if (arg == "inTest") {
                Configuration::inTest = true;
                printFlow("inTest on");
            }
            else if (arg == "traceOn") {
                Configuration::traceOn = true;
                printFlow("traceOn true");
            }
            else if (arg == "searchTree") {
                Configuration::searchTree = true;
                printFlow("searchTree on");
            }
            else if (arg == "--filter") {
                filterNext = true;
            }
            else {
                if (filterNext) {
                    filterNext = false;
                    Configuration::filterExpression = arg;
                }
                else {
                    Configuration::featureFiles.push_back(arg);
                }
            }
        }
    }

   
} // namespace gherkinexecutor

int main(int argc, char** args) {
    gherkinexecutor::Translate::printFlow("Gherkin Executor");
    gherkinexecutor::Configuration::printAllConfiguration();
    gherkinexecutor::Configuration::readFromYamlFile("Configuration.yaml");
    gherkinexecutor::Configuration::printAllConfiguration();
    gherkinexecutor::Translate::printFlow("Read Configuration File");
    gherkinexecutor::Configuration::currentDirectory = fs::current_path().string();
    gherkinexecutor::Translate::printFlow(gherkinexecutor::Configuration::currentDirectory);

    std::vector<std::string> argVec;
    for (int i = 1; i < argc; ++i) {
        argVec.emplace_back(args[i]);
    }
    gherkinexecutor::Translate::processArguments(argVec);
    if (gherkinexecutor::Configuration::searchTree && !gherkinexecutor::Configuration::startingFeatureDirectory.empty()) {
        std::vector<std::string> filesInTree = gherkinexecutor::Translate::findFeatureFiles(gherkinexecutor::Configuration::startingFeatureDirectory);
        gherkinexecutor::Translate::printFlow("Adding directory tree files");
        for (const std::string& file : filesInTree) {
            std::cout << file << std::endl;
        }
        gherkinexecutor::Configuration::featureFiles.insert(gherkinexecutor::Configuration::featureFiles.end(),
            filesInTree.begin(), filesInTree.end());
    }

    gherkinexecutor::Translate::readOptionList();
    gherkinexecutor::Translate::readFilterList();
    gherkinexecutor::Translate::readFeatureList();

    for (const std::string& name : gherkinexecutor::Configuration::featureFiles) {
        gherkinexecutor::Translate translate;
        gherkinexecutor::Translate::printFlow("Translating " + name);
        translate.translateInTests(name);
    }
    return 0;
}
