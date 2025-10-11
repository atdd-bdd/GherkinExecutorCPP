#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <memory>
#include "DataConstruct.h"
#include "Configuration.h"
#include "Translate.h"
namespace gherkinexecutor {

    class Translate;

    // Static member initialization
    const std::string DataConstruct::throwString = "";
    bool DataConstruct::oneDataFileStarted = false;

    // Implementation
    void DataConstruct::actOnData(const std::vector<std::string>& words) {
        if (words.size() < 2) {
            parent->error("Need to specify data class name");
            return;
        }

        std::string className = words[1];
        std::string internalClassName;
        bool providedOtherClassName;

        if (words.size() > 2) {
            internalClassName = words[2];
            providedOtherClassName = true;
        }
        else {
            providedOtherClassName = false;
            internalClassName = className + "Internal";
        }

        auto follow = parent->lookForFollow();
        std::string followType = follow.first;
        std::vector<std::string> table = follow.second;

        if (followType != "TABLE") {
            parent->error("Error table does not follow data " + words[0] + " " + words[1]);
            return;
        }

        if (parent->dataNames.find(className) != parent->dataNames.end()) {
            className += std::to_string(parent->stepCount);
            parent->warning("Data name is duplicated, has been renamed " + className);
        }

        parent->trace("Creating class for " + className);
        parent->dataNames[className] = "";

        // Put each in a new file
        startDataFile(className, false);
        parent->dataHeaderPrint("#include \"" + className + ".h\"");
        parent->dataHeaderPrint("#include \"" + internalClassName + ".h\"");
        dataPrintHeader("#pragma once");
        dataPrintLn("#include <algorithm>");
        dataPrintLn("#include <sstream>");
        dataPrintLn("#include <iostream>");
        dataPrintHeader("#include <string>");
        dataPrintHeader("#include <vector>");

        for (const std::string& line : parent->linesToAddForDataAndGlue) {
            dataPrintLn(line);
            dataPrintHeader(line);
        }
        dataPrintHeader("#include \""+ internalClassName +".h\""); 
        //dataPrintLn("#include \"" + parent->dataHeaderFilename + "\"");
        dataPrintLn("#include \"" + className + ".h\"");
        processNamespaces(parent->packagePath);
        addSuppressionOfWarnings();
        dataPrintHeader("class " + internalClassName + ";");
        dataPrintHeader("class " + className + " {");
        dataPrintHeader("public:");



        std::vector<DataValues> variables;
        bool doInternal;
        createVariableList(table, variables, doInternal);

        for (const DataValues& variable : variables) {
            parent->classDataNames.push_back(className + "#" + variable.name);
            dataPrintHeader("    std::string " + parent->makeName(variable.name) + " = \"" + variable.defaultVal + "\";");
        }

        createConstructor(variables, className);
        createEqualsMethod(variables, className);
        createBuilderMethod(variables, className);
        createToStringMethod(variables, className);
        createToJSONMethod(variables, className);
        createFromJSONMethod(variables, className);
        createTableToJSONMethod(className);
        createJSONToTableMethod(className);
        createMergeMethod(variables, className);

        if (doInternal) {
            parent->dataNamesInternal[internalClassName] = "";
            createConversionMethod(internalClassName, variables, className);
        }

        dataPrintHeader("};");
        endDataFile();

        if (doInternal) {
            createInternalClass(internalClassName, className, variables, providedOtherClassName);
        }
    }
    int DataConstruct::processNamespaces(const std::string& packagePath) {
        std::vector<std::string> parts = Translate::extractNamespaceParts(packagePath);

        for (const auto& part : parts) {
            dataPrintLn("namespace " + part + " {");
        }
        for (const auto& part : parts) {
            dataPrintHeader("namespace " + part + " {");
        }
        return static_cast<int>(parts.size());
    }
    void DataConstruct::endNamespace(const std::string& packagePath) {
        size_t count = std::count(packagePath.begin(), packagePath.end(), '.');

        for (size_t i = 0; i < count + 1; ++i) {
            dataPrintLn("}");
            dataPrintHeader("}");
        }
    }

    void DataConstruct::createVariableList(const std::vector<std::string>& table, std::vector<DataValues>& variables, bool& doInternal) {
        bool headerLine = true;
        doInternal = false;

        for (const std::string& line : table) {
            if (headerLine) {
                std::vector<std::string> headers = parent->parseLine(line);
                checkHeaders(headers);
                headerLine = false;

                if (headers.size() > 2) doInternal = true;
                continue;
            }

            std::vector<std::string> elements = parent->parseLine(line);
            if (elements.size() < 2) {
                parent->error(" Data line has less than 2 entries " + line);
                continue;
            }

            if (elements.size() > 3) {
                variables.emplace_back(parent->makeName(elements[0]), elements[1], alterDataType(elements[2]), elements[3]);
            }
            else if (elements.size() > 2) {
                variables.emplace_back(parent->makeName(elements[0]), elements[1], alterDataType(elements[2]));
            }
            else {
                variables.emplace_back(parent->makeName(elements[0]), elements[1]);
            }
        }
    }

    std::string DataConstruct::alterDataType(const std::string& s) {
        if (s == "Int") return "int";
        if (s == "Char") return "char";
        if (s == "Decimal") return "double";
        if (s == "string") return "std::string";
        if (s == "Text") return "std::string";
        if (s == "Boolean") return "bool";
        if (s == "boolean") return "bool";
        if (s == "Float") return "float";
        if (s == "Long") return "long";
        if (s == "Byte") return "uint8_t";
        if (s == "Short") return "short";
        if (s == "Integer") return "int";
        if (s == "Double") return "double";
        if (s == "String") return "std::string";

        return s;
    }

    void DataConstruct::checkHeaders(const std::vector<std::string>& headers) {
        std::vector<std::string> expected = { "Name", "Default", "Datatype", "Notes" };
        if (headers.size() < 2 || headers[0] != expected[0] || headers[1] != expected[1]) {
            parent->error("Headers should start with " + expected[0] + ", " + expected[1]);
        }
    }

    void DataConstruct::createConstructor(const std::vector<DataValues>& variables, const std::string& className) {
        //dataPrintLn("    " + className + "::" + className + "() {}");
        dataPrintHeader("    " + className + "();");
        dataPrintLn("    " + className + "::" + className + "(){}");
        dataPrintHeader("    " + className + "(");
        dataPrintLn("    " + className + "::" + className + "(");
        std::string comma = "";
        for (const DataValues& variable : variables) {
            dataPrintLn("        " + comma + "const std::string& " + parent->makeName(variable.name));
            dataPrintHeader("        " + comma + "const std::string& " + parent->makeName(variable.name));  
            comma = ",";
        }

        dataPrintLn("        ) {");
        dataPrintHeader("        ) ;");
        for (const DataValues& variable : variables) {
            dataPrintLn("        this->" + parent->makeName(variable.name) + " = " + parent->makeName(variable.name) + ";");
        }
        dataPrintLn("    }");
    }

    void DataConstruct::createInternalConstructor(const std::vector<DataValues>& variables, const std::string& className) {
        dataPrintLn("    " + className + "::" + className + "(");
        dataPrintHeader("    " + className + "(");
        std::string comma = "";
        for (const DataValues& variable : variables) {
            dataPrintLn("        " + comma + variable.dataType + " " + parent->makeName(variable.name));
            dataPrintHeader("        " + comma + variable.dataType + " " + parent->makeName(variable.name));    
            comma = ",";
        }

        dataPrintLn("        ) : ");
        dataPrintHeader("        ) ;");
        for (size_t i = 0; i < variables.size(); ++i) {
            const DataValues& variable = variables[i];
            std::string line = "        " + parent->makeName(variable.name) + "(" + parent->makeName(variable.name) + ")";
            if (i < variables.size() - 1) {
                line += ",";
            }
            dataPrintLn(line);
        }
        dataPrintLn("   { }");
    }

    void DataConstruct::createEqualsMethod(const std::vector<DataValues>& variables, const std::string& className) {
        dataPrintLn("    bool " + className + "::operator==(const " + className + " & other) const { ");
        dataPrintHeader("    bool operator==(const " + className + "& other) const ;");
        dataPrintLn("        if (this == &other) return true;");

        dataPrintLn("        bool result = true;");

        for (const DataValues& variable : variables) {
            dataPrintLn("        if (");
            dataPrintLn("            this->" + variable.name + " != " + parent->quoteIt(Configuration::doNotCompare));
            dataPrintLn("                && other." + variable.name + " != " + parent->quoteIt(Configuration::doNotCompare) + ")");
            dataPrintLn("                if (other." + variable.name + " != this->" + variable.name + ") result = false;");
        }
        dataPrintLn("        return result;");
        dataPrintLn("    }");
    }

    void DataConstruct::createInternalEqualsMethod(const std::vector<DataValues>& variables, const std::string& className) {
        dataPrintLn("    bool " + className + "::operator==(const " + className + " & other) const { ");
        dataPrintHeader("    bool operator==(const " + className + "& other) const ;");
        dataPrintLn("        if (this == &other) return true;");

        dataPrintLn("        return ");

        std::string andOperator = "";
        for (const DataValues& variable : variables) {
            std::string comparison = " == ";
            dataPrintLn("                " + andOperator + "(other." + variable.name + comparison + "this->" + variable.name + ")");
            andOperator = " && ";
        }
        dataPrintLn("        ;");
        dataPrintLn("    }");
    }

    bool DataConstruct::primitiveDataType(const DataValues& variable) {
        return (variable.dataType == "bool" || variable.dataType == "char" ||
            variable.dataType == "int" || variable.dataType == "float" ||
            variable.dataType == "double" || variable.dataType == "long" ||
            variable.dataType == "uint8_t" || variable.dataType == "short");
    }

    void DataConstruct::createBuilderMethod(const std::vector<DataValues>& variables, const std::string& className) {
        addSuppressionOfWarnings();
        dataPrintHeader("    class Builder {");
        dataPrintHeader("    private:");
       /* dataPrintHeader("    class Builder {");
        dataPrintHeader("    private:");*/

        for (const DataValues& variable : variables) {
            dataPrintHeader("        std::string " + variable.name + " = " + parent->quoteIt(variable.defaultVal) + ";");
        }

        dataPrintHeader("    public:");
        for (const DataValues& variable : variables) {
            dataPrintHeader("        Builder& " + parent->makeBuildName(variable.name) + "(const std::string& " + variable.name + ") {");
            dataPrintHeader("            this->" + variable.name + " = " + variable.name + ";");
            dataPrintHeader("            return *this;");
            dataPrintHeader("        }");
        }

        dataPrintHeader("        Builder& setCompare() {");
        for (const DataValues& variable : variables) {
            dataPrintHeader("            " + variable.name + " = " + parent->quoteIt(Configuration::doNotCompare) + ";");
        }
        dataPrintHeader("            return *this;");
        dataPrintHeader("        }");

        dataPrintHeader("        " + className + " build() {");
        dataPrintHeader("            return " + className + "(");

        std::string comma = "";
        for (const DataValues& variable : variables) {
            dataPrintHeader("                " + comma + variable.name);
            comma = ",";
        }
        dataPrintHeader("            );");
        dataPrintHeader("        }");
        dataPrintHeader("    };");
    }

    void DataConstruct::createToStringMethod(const std::vector<DataValues>& variables, const std::string& className) {
        dataPrintLn("    std::string " + className + "::to_string() const { ");
        dataPrintHeader("    std::string to_string() const ;");
        dataPrintLn("        return \"" + className + " {\"");

        for (const DataValues& variable : variables) {
            dataPrintLn("            \" " + variable.name + " = \" + " + variable.name + " + \" \"");
        }

        std::string endPart = "            + \"}\";";
        if (Configuration::addLineToString) {
            endPart = "            + \"}\\n\";";
        }
        dataPrintLn(endPart);
        dataPrintLn("    }");
    }

    void DataConstruct::createToStringMethodInternal(const std::vector<DataValues>& variables, const std::string& className) {
        dataPrintLn("    std::string " + className + "::to_string() const { ");
        dataPrintHeader("    std::string to_string() const ;");
        dataPrintLn("        return \"" + className + " {\"");

        for (const DataValues& variable : variables) {
            dataPrintLn("            \" " + variable.name + " = \" + " + makeValueToString(variable, true) + " + \" \"");
        }

        std::string endPart = "            + \"}\";";
        if (Configuration::addLineToString) {
            endPart = "            + \"}\\n\";";
        }
        dataPrintLn(endPart);
        dataPrintLn("    }");
    }

    void DataConstruct::createToJSONMethod(const std::vector<DataValues>& variables
        , const std::string& className) {
        dataPrintLn("    std::string " + className + "::toJson() const { ");
        dataPrintHeader("    std::string toJson() const ;");
        dataPrintLn("        return \"{\"");

        std::string comma = "";
        for (const DataValues& variable : variables) {
            dataPrintLn("            " + comma + "\"\\\"" + variable.name + "\\\":\" \"\\\"\" + " + variable.name + " + \"\\\"\"");
            comma = "\",\" + ";
        }
        dataPrintLn("            + \"}\";");
        dataPrintLn("    }");
    }

    void DataConstruct::createFromJSONMethod(const std::vector<DataValues>& variables, const std::string& className) {
        dataPrintLn("    " + className + " " + className + "::fromJson(const std::string & json) { ");
        dataPrintHeader("    static " + className + " fromJson(const std::string& json) ;");
        dataPrintLn("        " + className + " instance;");
        dataPrintLn("        ");
        dataPrintLn("        std::string cleanJson = json;");
        dataPrintLn("        cleanJson.erase(std::remove_if(cleanJson.begin(), cleanJson.end(), ::isspace), cleanJson.end());");
        dataPrintLn("        cleanJson.erase(std::remove(cleanJson.begin(), cleanJson.end(), '{'), cleanJson.end());");
        dataPrintLn("        cleanJson.erase(std::remove(cleanJson.begin(), cleanJson.end(), '}'), cleanJson.end());");
        dataPrintLn("        ");
        dataPrintLn("        std::stringstream ss(cleanJson);");
        dataPrintLn("        std::string pair;");
        dataPrintLn("        ");
        dataPrintLn("        while (std::getline(ss, pair, ',')) {");
        dataPrintLn("            size_t colonPos = pair.find(':');");
        dataPrintLn("            if (colonPos == std::string::npos) continue;");
        dataPrintLn("            ");
        dataPrintLn("            std::string key = pair.substr(0, colonPos);");
        dataPrintLn("            std::string value = pair.substr(colonPos + 1);");
        dataPrintLn("            ");
        dataPrintLn("            key.erase(std::remove(key.begin(), key.end(), '\\\"'), key.end());");
        dataPrintLn("            value.erase(std::remove(value.begin(), value.end(), '\\\"'), value.end());");
        dataPrintLn("            ");

        bool firstCase = true;
        for (const DataValues& variable : variables) {
            std::string ifElse = firstCase ? "if" : "else if";
            dataPrintLn("            " + ifElse + " (key == \"" + variable.name + "\") {");
            dataPrintLn("                instance." + variable.name + " = value;");
            dataPrintLn("            }");
            firstCase = false;
        }

        dataPrintLn("            else {");
        dataPrintLn("                std::cerr << \"Invalid JSON element: \" << key << std::endl;");
        dataPrintLn("            }");
        dataPrintLn("        }");
        dataPrintLn("        return instance;");
        dataPrintLn("    }");
    }

    void DataConstruct::createTableToJSONMethod(const std::string& className) {
        dataPrintHeader("    static std::string " + className + "::listToJson(const std::vector<" + className + ">&list); "); 
        std::string code = R"(    std::string listToJson(const std::vector<CLASSNAME>& list) {
        std::stringstream jsonBuilder;
        jsonBuilder << "[";
        
        for (size_t i = 0; i < list.size(); i++) {
            jsonBuilder << list[i].toJson();
            if (i < list.size() - 1) {
                jsonBuilder << ",";
            }
        }
        
        jsonBuilder << "]";
        return jsonBuilder.str();
    })";

        size_t pos = 0;
        while ((pos = code.find("CLASSNAME")) != std::string::npos) {
            code.replace(pos, 9, className);
        }
        dataPrintLn(code);
    }

    void DataConstruct::createJSONToTableMethod(const std::string& className) {
        std::string code = "";
        //dataPrintHeader("    static std::vector<" + className + "> listFromJson(const std::string& json) ;");   
        /*R"(    static std::vector<CLASSNAME> listFromJson(const std::string& json) {
            std::vector<CLASSNAME> list;
            // JSON parsing implementation
            return list;
        })";*/

        size_t pos = 0;
        while ((pos = code.find("CLASSNAME")) != std::string::npos) {
            code.replace(pos, 9, className);
        }
        //dataPrintLn(code);
    }

    void DataConstruct::createMergeMethod(const std::vector<DataValues>& variables, const std::string& className) {
        dataPrintHeader("    static std::vector<" + className + "> " + className + "::merge(const std::vector<" + className + ">&values, const std::vector<" + className + ">&toMerge); ");  
        dataPrintLn("    static std::vector<" + className + "> merge(const std::vector<" + className + ">& values, const std::vector<" + className + ">& toMerge) {");
        dataPrintLn("        " + className + " toAdd = toMerge[0];");
        dataPrintLn("        std::vector<" + className + "> result;");
        dataPrintLn("        for (const auto& value : values) {");
        dataPrintLn("            " + className + " current;");

        for (const DataValues& variable : variables) {
            dataPrintLn("            current." + variable.name + " = value." + variable.name + ";");
            dataPrintLn("            if (toAdd." + variable.name + " != \"?DNC?\") current." + variable.name + " = toAdd." + variable.name + ";");
        }

        dataPrintLn("            result.push_back(current);");
        dataPrintLn("        }");
        dataPrintLn("        return result;");
        dataPrintLn("    }");
    }

    void DataConstruct::createConversionMethod(const std::string& internalClassName, const std::vector<DataValues>& variables, 
        const std::string& className) {
        dataPrintHeader("    " + internalClassName + " to" + internalClassName + "() const; "); 
        dataPrintLn("    " + internalClassName + "  " + className + "::to" + internalClassName + "() const {");
        dataPrintLn("        return " + internalClassName + "(");

        std::string comma = "";
        for (const DataValues& variable : variables) {
            std::string initializer = parent->makeValueFromString(variable, true);
            dataPrintLn("            " + comma + initializer);
            comma = ",";
        }
        dataPrintLn("        );");
        dataPrintLn("    }");
    }

    void DataConstruct::createInternalClass(const std::string& className, const std::string& otherClassName,
        const std::vector<DataValues>& variables, bool providedClassName) {
        std::string classNameInternal = className;
        if (parent->dataNames.find(classNameInternal) != parent->dataNames.end()) {
            classNameInternal += std::to_string(parent->stepCount);
            parent->warning("Data name is duplicated, has been renamed " + classNameInternal);
        }

        parent->trace("Creating internal class for " + classNameInternal);
        parent->dataNames[classNameInternal] = "";
        startDataFile(className, providedClassName);
        dataPrintHeader("#pragma once");
        dataPrintHeader("#include <string>");
        dataPrintHeader("#include <vector>");

        for (const std::string& line : parent->linesToAddForDataAndGlue) {
            dataPrintLn(line);
            dataPrintHeader(line);
        }
         dataPrintLn("#include \"" + otherClassName + ".h\"");
        dataPrintLn("#include \"" + className + ".h\"");
        //dataPrintLn("#include \"" + parent->dataHeaderFilename + "\"");
        processNamespaces(parent->packagePath);
        dataPrintHeader("class " + otherClassName + ";");
        

        addSuppressionOfWarnings();

        dataPrintHeader("class " + className + " {");
        dataPrintHeader("public:");
        for (const DataValues& variable : variables) {
            dataPrintHeader("    " + variable.dataType + " " + parent->makeName(variable.name) + ";");  
        }
        dataPrintLn("    ");

        createDataTypeToStringObject(className, variables);
        createToStringObject(className, otherClassName, variables);
        createInternalConstructor(variables, className);
        createInternalEqualsMethod(variables, className);
        createToStringMethodInternal(variables, className);

        dataPrintHeader("};");
        endDataFile();
    }

    void DataConstruct::createDataTypeToStringObject(const std::string& className, const std::vector<DataValues>& variables) {
        dataPrintLn("    std::string " + className + "::toDataTypeString() {");
        dataPrintHeader("    static std::string toDataTypeString();");
        dataPrintLn("        return");

        dataPrintLn("            " + parent->quoteIt(className + " {"));

        for (const DataValues& variable : variables) {
            dataPrintLn("            " + parent->quoteIt(variable.dataType + " "));
        }

        dataPrintLn("            " + parent->quoteIt("} ") + ";");
        dataPrintLn("    }");
    }

    void DataConstruct::createToStringObject(const std::string& className, const std::string& otherClassName, const std::vector<DataValues>& variables) {
        dataPrintLn("    " + otherClassName + " " + className + "::to" + otherClassName + "() const {");
        dataPrintHeader("    " + otherClassName + " to" + otherClassName + "() const ;");   
        dataPrintLn("        return " + otherClassName + "(");

        std::string comma = "";
        for (const DataValues& variable : variables) {
            std::string method = makeValueToString(variable, true);
            dataPrintLn("            " + comma + method);
            comma = ",";
        }
        dataPrintLn("        );");
        dataPrintLn("    }");
    }

    std::string DataConstruct::makeValueToString(const DataValues& variable, bool makeNameValue) {
        std::string value;
        if (makeNameValue) {
            value = parent->makeName(variable.name);
        }
        else {
            value = parent->quoteIt(variable.defaultVal);
        }
      
        if (variable.dataType == "std::string") {
            return value;
        }
        else if (variable.dataType == "int" || variable.dataType == "double" ||
            variable.dataType == "uint8_t" || variable.dataType == "short" ||
            variable.dataType == "long" || variable.dataType == "float" ||
            variable.dataType == "bool" || variable.dataType == "char") {
            return "std::to_string(" + value + ")";
        }
        else {
            return value + ".to_string()";      
        }
    }

    void DataConstruct::startDataFile(const std::string& className, bool createTmpl) {
        if (Configuration::oneDataFile) {
            startOneDataFile();
            return;
        }

        std::string extension = Configuration::dataDefinitionFileExtension;
        if (createTmpl) extension = "tmpl";

        std::string dataDefinitionPathname = Configuration::testSubDirectory + parent->featureDirectory +
            parent->featureName + "/" + className + "." + extension;

        try {
            dataDefinitionFile = std::make_unique<std::ofstream>(dataDefinitionPathname);
        }
        catch (const std::exception& e) {
            parent->error("IO Exception in setting up the files");
            parent->error(" Writing " + dataDefinitionPathname);
        }
        std::string dataDefinitionHeaderPathname = Configuration::testSubDirectory + parent->featureDirectory +
            parent->featureName + "/" + className + "." + "h";

        try {
            dataDefinitionFileHeader = std::make_unique<std::ofstream>(dataDefinitionHeaderPathname);
        }
        catch (const std::exception& e) {
            parent->error("IO Exception in setting up the files");
            parent->error(" Writing " + dataDefinitionHeaderPathname);
        }
    }

    void DataConstruct::startOneDataFile() {
        if (oneDataFileStarted) return;
        oneDataFileStarted = true;

        std::string extension = Configuration::dataDefinitionFileExtension;
        std::string dataDefinitionPathname = Configuration::testSubDirectory + parent->featureDirectory +
            parent->featureName + "/" + parent->featureName + "_data." + extension;

        try {
            dataDefinitionFile = std::make_unique<std::ofstream>(dataDefinitionPathname);
        }
        catch (const std::exception& e) {
            parent->error("IO Exception in setting up the files");
            parent->error(" Writing " + dataDefinitionPathname);
        }
        std::string dataDefinitionHeaderPathname = Configuration::testSubDirectory + parent->featureDirectory +
            parent->featureName + "/" + parent->featureName + "_data." + "h";

        try {
            dataDefinitionFileHeader = std::make_unique<std::ofstream>(dataDefinitionHeaderPathname);
        }
        catch (const std::exception& e) {
            parent->error("IO Exception in setting up the files");
            parent->error(" Writing " + dataDefinitionHeaderPathname);
        }
    }

    void DataConstruct::endDataFile() {
        if (Configuration::oneDataFile) return;
        endNamespace(parent->packagePath);
        try {
            if (dataDefinitionFile) {
                dataDefinitionFile->close();
            }
        }
        catch (const std::exception& e) {
            // Handle error
        }
        try {
            if (dataDefinitionFileHeader) {
                dataDefinitionFile->close();
            }
        }
        catch (const std::exception& e) {
            // Handle error
        }

    }

    void DataConstruct::endOneDataFile() {
        if (!Configuration::oneDataFile || !oneDataFileStarted) return;
        endNamespace(parent->packagePath);
        try {
            if (dataDefinitionFile) {
                dataDefinitionFile->close();
                oneDataFileStarted = false;
            }
        }
        catch (const std::exception& e) {
            // Handle error
        }
        try {
            if (dataDefinitionFileHeader) {
                dataDefinitionFileHeader->close();
                oneDataFileStarted = false;
            }
        }
        catch (const std::exception& e) {
            // Handle error
        }
    }

    void DataConstruct::addSuppressionOfWarnings() {
        dataPrintLn("// Suppress warnings");
     }

    void DataConstruct::dataPrintLn(const std::string& line) {
        try {
            if (dataDefinitionFile) {
                *dataDefinitionFile << line << std::endl;
            }
        }
        catch (const std::exception& e) {
            parent->error("IO ERROR");
        }
    }

    void DataConstruct::dataPrintHeader(const std::string& line) {
        try {
            if (dataDefinitionFileHeader) {
                *dataDefinitionFileHeader << line << std::endl;
            }
        }
        catch (const std::exception& e) {
            parent->error("IO ERROR");
        }
    }

} // namespace gherkinexecutor