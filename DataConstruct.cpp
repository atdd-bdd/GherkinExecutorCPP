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
    bool DataConstruct::oneDataFileHeaderWritten = false;

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
            doInternal = false; 
        }
        else {
            providedOtherClassName = false;
            internalClassName = className + "Internal";
            doInternal = true;      
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
        std::vector<DataValues> variables;

        createVariableList(table, variables, doInternal);

        // Put each in a new file
        startDataFile(className, false);
        if (!Configuration::oneDataFile)  {


        parent->dataHeaderPrint("#include \"" + className + ".h\"");
        parent->dataHeaderPrint("#include \"" + internalClassName + ".h\"");
            }
        if (!Configuration::oneDataFile || !oneDataFileHeaderWritten) {
            std::cout << "*** Writing header of cpp and h file " << std::endl;
            dataPrintHeader("#pragma once");
            dataPrintLn("#include <algorithm>");
            dataPrintLn("#include <sstream>");
            dataPrintLn("#include <iostream>");
            dataPrintHeader("#include <string>");
            dataPrintHeader("#include <vector>");
            if (Configuration::oneDataFile)
                dataPrintLn("#include \"" + parent->featureName + "_data." + "h\"");
            for (const std::string& line : parent->linesToAddForDataAndGlue) {
                dataPrintLn(line);
                dataPrintHeader(line);
            }
        }
        if (!Configuration::oneDataFile) {
            dataPrintLn("#include \"" + internalClassName + ".h\"");
            dataPrintHeader("#include \"" + internalClassName + ".h\"");

            //dataPrintLn("#include \"" + parent->dataHeaderFilename + "\"");
            dataPrintLn("#include \"" + className + ".h\"");
        }
        if (!Configuration::oneDataFile || !oneDataFileHeaderWritten) {
            processNamespaces(parent->packagePath);
            oneDataFileHeaderWritten = true;
        }
        addSuppressionOfWarnings();
        dataPrintHeader("class " + internalClassName + ";");
        dataPrintHeader("class " + className + " {");
        dataPrintHeader("public:");




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
        else 
            createInternalClassEmpty(internalClassName, className, variables, providedOtherClassName);
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
       

        for (const std::string& line : table) {
            if (headerLine) {
                std::vector<std::string> headers = parent->parseLine(line);
                checkHeaders(headers);
                headerLine = false;

                
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
                variables.emplace_back(parent->makeName(elements[0]), elements[1], "std::string");
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
        if (s == "Byte") return "unsigned char";
        if (s == "byte") return "unsigned char";
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
        return (
            variable.dataType == "bool" ||
            variable.dataType == "char" ||
            variable.dataType == "signed char" ||
            variable.dataType == "unsigned char" ||
            variable.dataType == "short" ||
            variable.dataType == "unsigned short" ||
            variable.dataType == "int" ||
            variable.dataType == "unsigned int" ||
            variable.dataType == "long" ||
            variable.dataType == "unsigned long" ||
            variable.dataType == "long long" ||
            variable.dataType == "unsigned long long" ||
            variable.dataType == "float" ||
            variable.dataType == "double" ||
            variable.dataType == "long double"
            );
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

    void DataConstruct::createToJSONMethod(const std::vector<DataValues>& variables, const std::string& className) {
        dataPrintHeader("    std::string toJson() const;");
        dataPrintLn("    std::string " + className + "::toJson() const {");
        dataPrintLn("        std::ostringstream oss;");
        dataPrintLn("        oss << \"{\";");

        for (size_t i = 0; i < variables.size(); ++i) {
            const auto& var = variables[i];
            std::string comma = (i > 0) ? " << \",\" " : "";
            dataPrintLn("        oss" + comma + " << \"\\\"" + var.name + "\\\":\" << \"\\\"\" << " + var.name + " << \"\\\"\";");
        }

        dataPrintLn("        oss << \"}\";");
        dataPrintLn("        return oss.str();");
        dataPrintLn("    }");
    }

    void DataConstruct::createFromJSONMethod(const std::vector<DataValues>& variables, const std::string& className) {
        dataPrintHeader("    static " + className + " fromJson(const std::string& json);");
        dataPrintLn("    " + className + " " + className + "::fromJson(const std::string& json) {");
        dataPrintLn("        " + className + " instance;");
        dataPrintLn("        std::string cleanJson = json;");
        dataPrintLn("        cleanJson.erase(std::remove_if(cleanJson.begin(), cleanJson.end(), ::isspace), cleanJson.end());");
        dataPrintLn("        cleanJson.erase(std::remove(cleanJson.begin(), cleanJson.end(), '{'), cleanJson.end());");
        dataPrintLn("        cleanJson.erase(std::remove(cleanJson.begin(), cleanJson.end(), '}'), cleanJson.end());");
        dataPrintLn("        std::stringstream ss(cleanJson);");
        dataPrintLn("        std::string pair;");
        dataPrintLn("        while (std::getline(ss, pair, ',')) {");
        dataPrintLn("            size_t colonPos = pair.find(':');");
        dataPrintLn("            if (colonPos == std::string::npos) continue;");
        dataPrintLn("            std::string key = pair.substr(0, colonPos);");
        dataPrintLn("            std::string value = pair.substr(colonPos + 1);");
        dataPrintLn("            key.erase(std::remove(key.begin(), key.end(), '\\\"'), key.end());");
        dataPrintLn("            value.erase(std::remove(value.begin(), value.end(), '\\\"'), value.end());");

        bool first = true;
        for (const auto& var : variables) {
            std::string prefix = first ? "if" : "else if";
            dataPrintLn("            " + prefix + " (key == \"" + var.name + "\") {");
            dataPrintLn("                instance." + var.name + " = value;");
            dataPrintLn("            }");
            first = false;
        }

        dataPrintLn("            else {");
        dataPrintLn("                std::cerr << \"Invalid JSON element: \" << key << std::endl;");
        dataPrintLn("            }");
        dataPrintLn("        }");
        dataPrintLn("        return instance;");
        dataPrintLn("    }");
    }

    void DataConstruct::createTableToJSONMethod(const std::string& className) {
        dataPrintHeader("    static std::string listToJson(const std::vector<" + className + ">& list);");
        dataPrintLn("    std::string " + className + "::listToJson(const std::vector<" + className + ">& list) {");
        dataPrintLn("        std::ostringstream oss;");
        dataPrintLn("        oss << \"[\";");
        dataPrintLn("        for (size_t i = 0; i < list.size(); ++i) {");
        dataPrintLn("            oss << list[i].toJson();");
        dataPrintLn("            if (i + 1 < list.size()) oss << \",\";");
        dataPrintLn("        }");
        dataPrintLn("        oss << \"]\";");
        dataPrintLn("        return oss.str();");
        dataPrintLn("    }");
    }
    void DataConstruct::createJSONToTableMethod(const std::string& className) {
        dataPrintHeader("    static std::vector<" + className + "> fromJsonList(const std::string& json);");
        dataPrintLn("    std::vector<" + className + "> " + className + "::fromJsonList(const std::string& json) {");
        dataPrintLn("        std::vector<" + className + "> result;");
        dataPrintLn("        std::string clean = json;");
        dataPrintLn("        clean.erase(std::remove_if(clean.begin(), clean.end(), ::isspace), clean.end());");
        dataPrintLn("        std::string buffer;");
        dataPrintLn("        int braceDepth = 0;");
        dataPrintLn("        bool inObject = false;");
        dataPrintLn("        for (char ch : clean) {");
        dataPrintLn("            if (ch == '{') {");
        dataPrintLn("                if (!inObject) {");
        dataPrintLn("                    buffer.clear();");
        dataPrintLn("                    inObject = true;");
        dataPrintLn("                }");
        dataPrintLn("                ++braceDepth;");
        dataPrintLn("            }");
        dataPrintLn("            if (inObject) buffer += ch;");
        dataPrintLn("            if (ch == '}') {");
        dataPrintLn("                --braceDepth;");
        dataPrintLn("                if (braceDepth == 0 && inObject) {");
        dataPrintLn("                    result.push_back(" + className + "::fromJson(buffer));");
        dataPrintLn("                    inObject = false;");
        dataPrintLn("                }");
        dataPrintLn("            }");
        dataPrintLn("        }");
        dataPrintLn("        return result;");
        dataPrintLn("    }");
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
        if (!Configuration::oneDataFile) {
            dataPrintHeader("#pragma once");
            dataPrintHeader("#include <string>");
            dataPrintHeader("#include <vector>");

            for (const std::string& line : parent->linesToAddForDataAndGlue) {
                dataPrintLn(line);
                dataPrintHeader(line);
            }
            dataPrintLn("#include \"" + otherClassName + ".h\"");
            dataPrintLn("#include \"" + className + ".h\"");
            processNamespaces(parent->packagePath);
        }
        //dataPrintLn("#include \"" + parent->dataHeaderFilename + "\"");
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
    void DataConstruct::createInternalClassEmpty(const std::string& className, const std::string& otherClassName,
        const std::vector<DataValues>& variables, bool providedClassName) {
        std::cout << "** skipping creating internal class for " << className << std::endl;
        return;
        std::string classNameInternal = className;
        if (parent->dataNames.find(classNameInternal) != parent->dataNames.end()) {
            classNameInternal += std::to_string(parent->stepCount);
            parent->warning("Data name is duplicated, has been renamed " + classNameInternal);
        }

        parent->trace("Creating empty internal class for " + classNameInternal);
        parent->dataNames[classNameInternal] = "";
        startDataFile(className, providedClassName);
        dataPrintHeader("#pragma once");
        processNamespaces(parent->packagePath);
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
        else if (variable.dataType == "bool") {
            return "std::string(" + value + " ? \"true\" : \"false\")";
        }
        else if (variable.dataType == "char" || variable.dataType == "signed char" || variable.dataType == "unsigned char") {
            return "std::string(1, " + value + ")";
        }
        else if (variable.dataType == "short" || variable.dataType == "unsigned short") {
            return "std::to_string(static_cast<int>(" + value + "))";
        }
        else if (
            variable.dataType == "int" || variable.dataType == "unsigned int" ||
            variable.dataType == "long" || variable.dataType == "unsigned long" ||
            variable.dataType == "long long" || variable.dataType == "unsigned long long" ||
            variable.dataType == "float" || variable.dataType == "double" || variable.dataType == "long double"
            ) {
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
        if (createTmpl) extension = "cpp.tmpl";

        std::string dataDefinitionPathname = Configuration::testSubDirectory + parent->featureDirectory +
            parent->featureName + "/" + className + "_" + parent->featureName + "." + extension;

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
        std::cout << "** Starting one data file" << std::endl;

        oneDataFileStarted = true;
        oneDataFileHeaderWritten = false; 

        std::string extension = Configuration::dataDefinitionFileExtension;
        std::string dataDefinitionPathname = Configuration::testSubDirectory + parent->featureDirectory +
            parent->featureName + "/" + parent->featureName + "_data." + extension;
        std::cout << "** Data file " << dataDefinitionPathname << std::endl;    
        try {
            dataDefinitionFile = std::make_unique<std::ofstream>(dataDefinitionPathname);
        }
        catch (const std::exception& e) {
            parent->error("IO Exception in setting up the files");
            parent->error(" Writing " + dataDefinitionPathname);
        }
        std::string dataDefinitionHeaderPathname = Configuration::testSubDirectory + parent->featureDirectory +
            parent->featureName + "/" + parent->featureName + "_data." + "h";
        std::cout << "** Data header file " << dataDefinitionHeaderPathname << std::endl;    
         try {
            dataDefinitionFileHeader = std::make_unique<std::ofstream>(dataDefinitionHeaderPathname);
        }
        catch (const std::exception& e) {
            parent->error("IO Exception in setting up the files");
            parent->error(" Writing " + dataDefinitionHeaderPathname);
        }
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