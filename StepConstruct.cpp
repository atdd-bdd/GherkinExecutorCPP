



#include "StepConstruct.h"
#include "Translate.h"
// Implementation
namespace gherkinexecutor {
    void StepConstruct::actOnStep(const std::string& fullName, const std::vector<std::string>& comment) {
        getStepNumberInScenario()++;
        auto follow = lookForFollow();
        std::string followType = follow.first;
        std::vector<std::string> table = follow.second;

        testPrint("");

        if (followType == "TABLE") {
            createTheTable(comment, table, fullName);
        }
        else if (followType == "NOTHING") {
            noParameter(fullName);
        }
        else if (followType == "STRING") {
            createTheStringCode(comment, table, fullName);
        }
        else {
            error("Internal Error - Follow type " + followType);
        }
    }

    void StepConstruct::createTheStringCode(const std::vector<std::string>& comment,
        const std::vector<std::string>& table,
        const std::string& fullName) {
        std::string option = "String";
        if (!comment.empty() && !comment[0].empty()) {
            option = comment[0];
        }

        if (option == "ListOfString") {
            stringToList(table, fullName);
        }
        else {
            stringToString(table, fullName);
        }
    }

    void StepConstruct::stringToList(const std::vector<std::string>& table, const std::string& fullName) {
        std::string s = std::to_string(getStepNumberInScenario());
        std::string dataType = "std::vector<std::string>";

        testPrint("        std::vector<std::string> stringList" + s + " = {");
        std::string comma = "";
        for (const auto& line : table) {
            testPrint("            " + comma + "\"" + line + "\"");
            comma = ",";
        }
        testPrint("            };");
        testPrint("        " + getGlueObject() + "." + fullName + "(stringList" + s + ");");
        getTemplateConstruct().makeFunctionTemplateIsList(dataType, fullName, "std::string");
    }

    void StepConstruct::stringToString(const std::vector<std::string>& table, const std::string& fullName) {
        std::string s = std::to_string(getStepNumberInScenario());
        testPrint("        std::string string" + s + " = ");
        std::string prefix = "R\"(";
        for (const auto& line : table) {
            testPrint(prefix + line);
            prefix = ""; 
        }
        testPrint(")\";");
        testPrint("        " + getGlueObject() + "." + fullName + "(string" + s + ");");
        getTemplateConstruct().makeFunctionTemplate("std::string", fullName);
    }

    void StepConstruct::tableToListOfListOfObject(const std::vector<std::string>& table,
        const std::string& fullName,
        const std::string& className) {
    
        std::string s = std::to_string(getStepNumberInScenario());
        std::string dataType = "std::vector<std::vector<std::string>>";

        testPrint("        std::vector<std::vector<std::string>> stringListList" + s + " = {");
        std::string comma = "";
        for (const auto& line : table) {
            convertBarLineToList(line, comma);
            comma = ",";
        }
        testPrint("            };");
        testPrint("        " + getGlueObject() + "." + fullName + "(stringListList" + s + ");");
        getTemplateConstruct().makeFunctionTemplateObject(dataType, fullName, className);
        createConvertTableToListOfListOfObjectMethod(className);
    }

    void StepConstruct::tableToListOfList(const std::vector<std::string>& table, const std::string& fullName) {
        std::string s = std::to_string(getStepNumberInScenario());
        std::string dataType = "std::vector<std::vector<std::string>>";

        testPrint("        std::vector<std::vector<std::string>> stringListList" + s + " = {");
        std::string comma = "";
        for (const auto& line : table) {
            convertBarLineToList(line, comma);
            comma = ",";
        }
        testPrint("            };");
        testPrint("        " + getGlueObject() + "." + fullName + "(stringListList" + s + ");");
        getTemplateConstruct().makeFunctionTemplateIsList(dataType, fullName, "std::vector<std::string>");
    }

    void StepConstruct::createTheTable(const std::vector<std::string>& comment,
        const std::vector<std::string>& table,
        const std::string& fullName) {
        std::string option = "ListOfList";
        std::string className;

        if (!comment.empty() && !comment[0].empty()) {
            option = comment[0];
        }

        if (option == "ListOfList") {
            tableToListOfList(table, fullName);
        }
        else if (option == "ListOfListOfObject") {
            if (comment.size() < 2) {
                error("No class name specified");
                return;
            }
            className = comment[1];
            tableToListOfListOfObject(table, fullName, className);
        }
        else if (option == "String" || option == "string") {
            tableToString(table, fullName);
        }
        else if (option == "ListOfObject") {
            if (comment.size() < 2) {
                error("No class name specified");
                return;
            }
            className = comment[1];
            bool transpose = false;
            bool compare = false;

            if (comment.size() > 2) {
                std::string action = comment[2];
                if (action == "compare" || action == "Compare") {
                    compare = true;
                }
                else if (!(action == "transpose" || action == "Transpose")) {
                    error("Action not recognized " + action);
                }
                else {
                    transpose = true;
                }
            }
            tableToListOfObject(table, fullName, className, transpose, compare);
        }
        else {
            error("Option not found, default to ListOfList " + option);
            tableToListOfList(table, fullName);
        }
    }

    void StepConstruct::createConvertTableToListOfListOfObjectMethod(const std::string& toClassOriginal) {
        // Note: This would need DataConstruct::DataValues equivalent in C++
        outer->trace("Creating convertTableToListOfListOfObjectMethod for " + toClassOriginal);
        std::string toClass = this->outer->dataConstruct->alterDataType(toClassOriginal); 
        DataConstruct::DataValues variable("s", "default", toClass, "notes");   
        std::string convert = this->outer->makeValueFromString(variable, true);
        std::string template_str = R"(
                static std::vector<std::vector<)" + toClass + R"(>> convertListTo)" + toClass + R"((const std::vector<std::vector<std::string>>& stringList) {
                    std::vector<std::vector<)" + toClass + R"(>> classList;
                    for (const auto& innerList : stringList) {
                        std::vector<)" + toClass + R"(> innerClassList;
                        for (const auto& s : innerList) {
                            innerClassList.push_back()" + convert + R"();
                        }
                        classList.push_back(innerClassList);
                    }
                return classList;
                }
        )";

        getLinesToAddToEndOfGlue().push_back(template_str);
    }

    void StepConstruct::tableToString(const std::vector<std::string>& table, const std::string& fullName) {
        std::string s = std::to_string(getStepNumberInScenario());
        testPrint("        std::string table" + s + " = ");
        std::string prefix = "R\"(";
        for (const auto& line : table) {
            testPrint(prefix + line);
            prefix = "";
        }
        testPrint(")\";");
        testPrint("        " + getGlueObject() + "." + fullName + "(table" + s + ");");
        getTemplateConstruct().makeFunctionTemplate("std::string", fullName);
    }

    void StepConstruct::convertBarLineToList(const std::string& lineIn, const std::string& commaIn) {
        std::string line = lineIn;
        size_t pos = line.find('#');
        if (pos != std::string::npos) {
            line = line.substr(0, pos);
        }
        // Trim whitespace
        line.erase(0, line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t") + 1);

        testPrint("           " + commaIn + "{");
        std::vector<std::string> elements = parseLine(line);
        std::string comma = "";
        for (const auto& element : elements) {
            testPrint("            " + comma + "\"" + element + "\"");
            comma = ",";
        }
        testPrint("            }");
    }

    void StepConstruct::tableToListOfObject(const std::vector<std::string>& table,
        const std::string& fullName,
        const std::string& className,
        bool transpose,
        bool compare) {
        trace("TableToListOfObject classNames " + className);
        std::string s = std::to_string(getStepNumberInScenario());
        std::string dataType = "std::vector<" + className + ">";

        testPrint("        std::vector<" + className + "> objectList" + s + " = {");
        bool inHeaderLine = true;
        std::vector<std::vector<std::string>> dataList = convertToListList(table, transpose);
        std::vector<std::string> headers;
        std::string comma = "";

        for (const auto& row : dataList) {
            if (inHeaderLine) {
                headers = row;
                for (const auto& dataName : row) {
                    if (!findDataClassName(className, makeName(dataName))) {
                        error("Data name " + dataName + " not in Data for " + className);
                    }
                }
                inHeaderLine = false;
                continue;
            }

            convertBarLineToParameter(headers, row, className, comma, compare);
            comma = ",";
        }
        testPrint("            };");
        testPrint("        " + getGlueObject() + "." + fullName + "(objectList" + s + ");");

        getTemplateConstruct().makeFunctionTemplateIsList(dataType, fullName, className);
    }

    std::vector<std::vector<std::string>> StepConstruct::convertToListList(const std::vector<std::string>& table, bool transpose) {
        std::vector<std::vector<std::string>> temporary;
        for (const auto& line : table) {
            temporary.push_back(parseLine(line));
        }

        std::vector<std::vector<std::string>> result = temporary;
        if (transpose) {
            result = this->transpose(temporary);
        }
        return result;
    }

    bool StepConstruct::findDataClassName(const std::string& className, const std::string& dataName) {
        std::string compare = className + "#" + dataName;
        for (const auto& value : getClassDataNames()) {
            if (value == compare) {
                return true;
            }
        }
        return false;
    }

    void StepConstruct::convertBarLineToParameter(const std::vector<std::string>& headers,
        const std::vector<std::string>& values,
        const std::string& className,
        const std::string& comma,
        bool compare) {
        trace("Headers " + [&headers]() {
            std::string result;
            for (const auto& h : headers) {
                if (!result.empty()) result += ", ";
                result += h;
            }
            return result;
            }());

        size_t size = headers.size();
        if (headers.size() > values.size()) {
            size = values.size();
            error("not sufficient values, using what is there");
        }

        testPrint("            " + comma + " " + className + "::Builder()");
        if (compare) {
            testPrint("             .setCompare()");
        }

        for (size_t i = 0; i < size; i++) {
            std::string temp = values[i];
            std::replace(temp.begin(), temp.end(), Configuration::spaceCharacters, ' ');
            std::string value = "\"" + temp + "\"";
            testPrint("                ." + makeBuildName(headers[i]) + "(" + value + ")");
        }
        testPrint("                .build()");
    }

    void StepConstruct::noParameter(const std::string& fullName) {
        testPrint("        " + getGlueObject() + "." + fullName + "();");
        getTemplateConstruct().makeFunctionTemplateNothing("", fullName);
    }

    std::vector<std::vector<std::string>> StepConstruct::transpose(const std::vector<std::vector<std::string>>& matrix) {
        if (matrix.empty() || matrix[0].empty()) {
            return {};
        }

        std::vector<std::vector<std::string>> transposed;
        for (size_t i = 0; i < matrix[0].size(); i++) {
            std::vector<std::string> row;
            for (size_t j = 0; j < matrix.size(); j++) {
                row.push_back(matrix[j][i]);
            }
            transposed.push_back(row);
        }
        return transposed;
    }

    // Helper method implementations - these would need to delegate to the outer Translate class
    std::pair<std::string, std::vector<std::string>> StepConstruct::lookForFollow() {
        // Delegate to outer->lookForFollow()
        return outer->lookForFollow();
    }

    void StepConstruct::testPrint(const std::string& line) {
        // Delegate to outer->testPrint()
        outer->testPrint(line);
    }

    void StepConstruct::error(const std::string& message) {
        // Delegate to outer->error()
        outer->error(message);
    }

    void StepConstruct::trace(const std::string& message) {
        // Delegate to outer->trace()
        outer->trace(message);
    }

    std::vector<std::string> StepConstruct::parseLine(const std::string& line) {
        // Delegate to outer->parseLine()
        return outer->parseLine(line);
    }

    std::string StepConstruct::makeName(const std::string& input) {
        // Delegate to outer->makeName()
        return outer->makeName(input);
    }

    std::string StepConstruct::makeBuildName(const std::string& s) {
        // Delegate to outer->makeBuildName()
        return outer->makeBuildName(s);
    }

    int& StepConstruct::getStepNumberInScenario() {
        // Delegate to outer->stepNumberInScenario
        return outer->stepNumberInScenario;
    }

    std::string& StepConstruct::getGlueObject() {
        // Delegate to outer->glueObject
        return outer->glueObject;
    }

    TemplateConstruct& StepConstruct::getTemplateConstruct() {
        // Delegate to outer->templateConstruct
        return * outer->templateConstruct;
    }

    std::vector<std::string>& StepConstruct::getClassDataNames() {
        // Delegate to outer->classDataNames
        return outer->classDataNames;
    }

    std::vector<std::string>& StepConstruct::getLinesToAddToEndOfGlue() {
        // Delegate to outer->linesToAddToEndOfGlue
        return outer->linesToAddToEndOfGlue;
    }

}