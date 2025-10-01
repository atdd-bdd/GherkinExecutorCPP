//#include "Translate.h"
//#include <iostream>
//#include <fstream>
//#include <filesystem>
//#include <sstream>
//#include <algorithm>
//#include <cctype>
//
//namespace gherkinexecutor {
//
//    // Static member initializations
//    const std::string Translate::TAG_INDICATOR = "@";
//    const std::string InputIterator::EOF_STR = "EOF";
//
//    // Configuration static members
//    bool Configuration::logIt = false;
//    bool Configuration::inTest = false;
//    bool Configuration::traceOn = false;
//    const char Configuration::spaceCharacters = '~';
//    const bool Configuration::addLineToString = true;
//    const std::string Configuration::doNotCompare = "?DNC?";
//    std::string Configuration::currentDirectory = "";
//    const std::string Configuration::featureSubDirectory = "src/test/java/";
//    const std::string Configuration::treeDirectory = "features/";
//    const std::string Configuration::startingFeatureDirectory = Configuration::featureSubDirectory + Configuration::treeDirectory;
//    bool Configuration::searchTree = false;
//    const std::string Configuration::packageName = "gherkinexecutor";
//    const std::string Configuration::testSubDirectory = "src/test/java/" + Configuration::packageName + "/";
//    const std::string Configuration::dataDefinitionFileExtension = "java";
//    const std::string Configuration::testFramework = "JUnit5";
//    std::string Configuration::addToPackageName = "";
//    std::vector<std::string> Configuration::linesToAddForDataAndGlue;
//    std::string Configuration::filterExpression = "";
//    std::vector<std::string> Configuration::featureFiles;
//    std::string Configuration::tagFilter = "";
//    bool Configuration::oneDataFile = false;
//
//    // DataConstruct static members
//    const std::string DataConstruct::throwString = "";
//    bool DataConstruct::oneDataFileStarted = false;
//
//    void Configuration::initialize() {
//        linesToAddForDataAndGlue.push_back("import java.util.*;");
//    }
//
//    // Translate implementation
//    Translate::Translate()
//        : stepCount(0), stepNumberInScenario(0), firstScenario(true),
//        addBackground(false), addCleanup(false), inCleanup(false),
//        finalCleanup(false), featureActedOn(false), packagePath("Not Set"),
//        skipSteps(false), scenarioCount(0), backgroundCount(0), cleanupCount(0),
//        tagLineNumber(0), errorOccurred(false) {
//
//        filterExpression = Configuration::filterExpression;
//        linesToAddForDataAndGlue = Configuration::linesToAddForDataAndGlue;
//
//        dataConstruct = std::make_unique<DataConstruct>(this);
//        templateConstruct = std::make_unique<TemplateConstruct>(this);
//        stepConstruct = std::make_unique<StepConstruct>(this);
//        importConstruct = std::make_unique<ImportConstruct>(this);
//        defineConstruct = std::make_unique<DefineConstruct>(this);
//    }
//
//    Translate::~Translate() = default;
//
//    void Translate::translateInTests(const std::string& name) {
//        findFeatureDirectory(name);
//
//        linesToAddForDataAndGlue.insert(linesToAddForDataAndGlue.end(),
//            Configuration::linesToAddForDataAndGlue.begin(),
//            Configuration::linesToAddForDataAndGlue.end());
//
//        dataIn = std::make_unique<InputIterator>(name, featureDirectory, this);
//        alterFeatureDirectory();
//
//        if (dataIn->isEmpty())
//            return;
//
//        for (int pass = 1; pass <= 3; pass++) {
//            dataIn->reset();
//            bool eof = false;
//            while (!eof) {
//                std::string line = dataIn->next();
//                if (line == InputIterator::EOF_STR) {
//                    eof = true;
//                    continue;
//                }
//                actOnLine(line, pass);
//            }
//        }
//        endUp();
//    }
//
//    void Translate::findFeatureDirectory(const std::string& name) {
//        std::string directory = "";
//        size_t indexForward = name.find_last_of('/');
//        size_t indexBack = name.find_last_of('\\');
//        size_t index = std::max(indexForward, indexBack);
//
//        if (index != std::string::npos) {
//            directory = name.substr(0, index + 1);
//        }
//
//        featureDirectory = directory;
//        featurePackagePath = featureDirectory;
//        std::replace(featurePackagePath.begin(), featurePackagePath.end(), '\\', '.');
//        std::replace(featurePackagePath.begin(), featurePackagePath.end(), '/', '.');
//    }
//
//    void Translate::alterFeatureDirectory() {
//        std::string searchFor = Configuration::treeDirectory;
//        std::string alternateSearchFor = searchFor;
//        std::replace(alternateSearchFor.begin(), alternateSearchFor.end(), '/', '\\');
//
//        std::string directory = featureDirectory;
//        size_t pos = directory.find(searchFor);
//        if (pos != std::string::npos) {
//            directory.replace(pos, searchFor.length(), "");
//        }
//        pos = directory.find(alternateSearchFor);
//        if (pos != std::string::npos) {
//            directory.replace(pos, alternateSearchFor.length(), "");
//        }
//
//        featureDirectory = directory;
//        featurePackagePath = featureDirectory;
//        std::replace(featurePackagePath.begin(), featurePackagePath.end(), '\\', '.');
//        std::replace(featurePackagePath.begin(), featurePackagePath.end(), '/', '.');
//    }
//
//    std::string Translate::filterWord(const std::string& input) {
//        if (input.empty()) {
//            return "";
//        }
//
//        std::string result;
//        for (char c : input) {
//            if (std::isalnum(c) || c == '_' || c == '*') {
//                result += c;
//            }
//        }
//        return result;
//    }
//
//    std::string Translate::wordWithOutColon(const std::string& word) {
//        std::string result = word;
//        // Remove leading and trailing colons
//        while (!result.empty() && result.front() == ':') {
//            result.erase(0, 1);
//        }
//        while (!result.empty() && result.back() == ':') {
//            result.pop_back();
//        }
//        return result;
//    }
//
//    std::string Translate::wordWithOutHash(const std::string& word) {
//        std::string result = word;
//        // Remove leading and trailing hashes
//        while (!result.empty() && result.front() == '#') {
//            result.erase(0, 1);
//        }
//        while (!result.empty() && result.back() == '#') {
//            result.pop_back();
//        }
//        return result;
//    }
//
//    Pair<std::vector<std::string>, std::vector<std::string>> Translate::splitLine(const std::string& line, int pass) {
//        std::istringstream iss(line);
//        std::vector<std::string> allWords;
//        std::string word;
//
//        while (iss >> word) {
//            allWords.push_back(word);
//        }
//
//        std::vector<std::string> words;
//        std::vector<std::string> comment;
//
//        if ((pass == 3 || pass == 2) && line.find(TAG_INDICATOR) == 0) {
//            tagLine = line;
//            tagLineNumber = dataIn->getLineNumber();
//        }
//
//        bool inComment = false;
//        for (const std::string& aWord : allWords) {
//            std::string trimmedWord = aWord;
//            // Trim whitespace
//            trimmedWord.erase(0, trimmedWord.find_first_not_of(" \t"));
//            trimmedWord.erase(trimmedWord.find_last_not_of(" \t") + 1);
//
//            if (trimmedWord.empty()) continue;
//
//            if (!trimmedWord.empty() && trimmedWord.back() == ':') {
//                trimmedWord = wordWithOutColon(trimmedWord);
//            }
//
//            if (trimmedWord.empty()) continue;
//
//            if (inComment) {
//                comment.push_back(trimmedWord);
//                continue;
//            }
//
//            if (!trimmedWord.empty() && trimmedWord[0] == '#') {
//                inComment = true;
//                trimmedWord = wordWithOutHash(trimmedWord);
//                if (!trimmedWord.empty()) {
//                    comment.push_back(trimmedWord);
//                }
//                continue;
//            }
//
//            trimmedWord = filterWord(trimmedWord);
//            words.push_back(trimmedWord);
//        }
//
//        return Pair<std::vector<std::string>, std::vector<std::string>>(words, comment);
//    }
//
//    void Translate::actOnLine(const std::string& line, int pass) {
//        auto splitResult = splitLine(line, pass);
//        std::vector<std::string> words = splitResult.getFirst();
//        std::vector<std::string> comment = splitResult.getSecond();
//
//        if (!words.empty()) {
//            std::string keyword = wordWithOutColon(words[0]);
//            if (keyword == "*") {
//                keyword = "Star";
//            }
//            words[0] = keyword;
//            actOnKeyword(keyword, words, comment, pass);
//        }
//    }
//
//    std::string Translate::makeFullName(const std::vector<std::string>& words) {
//        std::string temp;
//        for (size_t i = 0; i < words.size(); ++i) {
//            if (i > 0) temp += "_";
//            temp += words[i];
//        }
//
//        std::regex pattern("[^A-Za-z0-9_]");
//        return std::regex_replace(temp, pattern, "_");
//    }
//
//    void Translate::actOnKeyword(const std::string& keyword, const std::vector<std::string>& words,
//        const std::vector<std::string>& comment, int pass) {
//        std::string fullName = makeFullName(words);
//        trace("Act on keyword " + keyword + " " + fullName + " pass " + std::to_string(pass));
//
//        std::string modifiedKeyword = keyword;
//        std::vector<std::string> modifiedWords = words;
//
//        if (keyword == "Star" && words.size() > 1) {
//            if (words[1] == "Data") {
//                modifiedKeyword = "Data";
//                modifiedWords.erase(modifiedWords.begin());
//            }
//            else if (words[1] == "Import") {
//                modifiedKeyword = "Import";
//                modifiedWords.erase(modifiedWords.begin());
//            }
//            else if (words[1] == "Define") {
//                modifiedKeyword = "Define";
//                modifiedWords.erase(modifiedWords.begin());
//            }
//            else if (words[1] == "Cleanup") {
//                modifiedKeyword = "Cleanup";
//                modifiedWords.erase(modifiedWords.begin());
//            }
//        }
//
//        if (modifiedKeyword == "Feature") {
//            if (pass != 2) return;
//            actOnFeature(fullName);
//            if (TagFilterEvaluator::shouldNotExecute(comment, filterExpression)) {
//                dataIn->goToEnd();
//                std::cout << " Skip Entire Feature " << std::endl;
//            }
//        }
//        else if (modifiedKeyword == "Scenario") {
//            if (pass != 3) return;
//            if (TagFilterEvaluator::shouldNotExecute(comment, filterExpression)) {
//                skipSteps = true;
//                return;
//            }
//            skipSteps = false;
//            actOnScenario(fullName);
//            inCleanup = false;
//        }
//        else if (modifiedKeyword == "Background") {
//            addBackground = true;
//            if (pass != 3) return;
//            skipSteps = false;
//            actOnBackground(fullName);
//            inCleanup = true;
//        }
//        else if (modifiedKeyword == "Cleanup") {
//            addCleanup = true;
//            if (pass != 3) return;
//            skipSteps = false;
//            actOnCleanup(fullName);
//            inCleanup = true;
//        }
//        else if (modifiedKeyword == "But" || modifiedKeyword == "Given" || modifiedKeyword == "When" ||
//            modifiedKeyword == "Then" || modifiedKeyword == "And" || modifiedKeyword == "Star" ||
//            modifiedKeyword == "Arrange" || modifiedKeyword == "Act" || modifiedKeyword == "Assert" ||
//            modifiedKeyword == "Rule" || modifiedKeyword == "Calculation" || modifiedKeyword == "Trigger" ||
//            modifiedKeyword == "Verify" || modifiedKeyword == "Assemble" || modifiedKeyword == "Activate" ||
//            modifiedKeyword == "Preconditions" || modifiedKeyword == "MainCourse" ||
//            modifiedKeyword == "Exception" || modifiedKeyword == "Postconditions") {
//            if (pass != 3) return;
//            if (skipSteps) return;
//            stepConstruct->actOnStep(fullName, comment);
//        }
//        else if (modifiedKeyword == "Data") {
//            if (pass != 2) return;
//            skipSteps = false;
//            dataConstruct->actOnData(modifiedWords);