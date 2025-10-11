#pragma once

#include <string>
#include <memory>
#include <fstream>
#include "TemplateConstruct.h"
#include "Configuration.h"
namespace gherkinexecutor {

    class Translate;

    // Implementation
    void TemplateConstruct::templatePrint(const std::string& line) {
        try {
            if (glueTemplateFile) {
                *glueTemplateFile << line << std::endl;
            }
        }
        catch (const std::exception& e) {
            parent->error("IO ERROR");
        }
    }

    void TemplateConstruct::templateHeaderPrint(const std::string& line) {
        try {
            if (glueTemplateFileHeader) {
                *glueTemplateFileHeader << line << std::endl;
            }
        }
        catch (const std::exception& e) {
            parent->error("IO ERROR");
        }
    }

    bool TemplateConstruct::checkForExistingTemplate(const std::string& dataType, const std::string& fullName) {
        auto it = parent->glueFunctions.find(fullName);
        if (it != parent->glueFunctions.end()) {
            if (it->second != dataType) {
                parent->error("function " + fullName + " datatype " + it->second + " not equals " + dataType);
                return true;
            }
            return true;
        }
        return false;
    }

    void TemplateConstruct::makeFunctionTemplate(const std::string& dataType, const std::string& fullName) {
        if (checkForExistingTemplate(dataType, fullName)) return;
        std::cout << "Creating function template for " << fullName << " with data type " << dataType << std::endl;   
        parent->glueFunctions[fullName] = dataType;
        templateHeaderPrint("   void " + fullName + "(" + dataType + " values); ");

        templatePrint("    void " + parent->glueClass + "::" + fullName + "(" + dataType + " value ) {");
        templatePrint("        std::cout << \"--- << " + fullName + "\" << std::endl;");

        if (Configuration::logIt) {
            templatePrint("        log(\"--- " + fullName + "\");");
            // templatePrint("        log(value.toString());");
        }

        templatePrint("        std::cout << value << std::endl;");

        if (!Configuration::inTest) {
            templatePrint("        fail(\"Must implement\");");
        }

        templatePrint("    }");
        templatePrint("");
    }

    void TemplateConstruct::makeFunctionTemplateIsList(const std::string& dataType, const std::string& fullName,
        const std::string& listElement) {
        if (checkForExistingTemplate(dataType, fullName)) return;
        std::cout << "Creating function template is list for " << fullName << " with data type " << dataType << std::endl;
        parent->glueFunctions[fullName] = dataType;
        templateHeaderPrint("   void " + fullName + "(const " + dataType + "& values); ");
        templatePrint("    void " + parent->glueClass + "::" + fullName + "(const " + dataType + "& values ) {");
        templatePrint("        std::cout << \"---  \" << \"" + fullName + "\" << std::endl;");

        if (dataType == "std::vector<std::vector<std::string>>") {
            // Special handling for vector of vector of strings
            templatePrint("        for (const auto& row : values) {");
            templatePrint("            for (const auto& element : row) {");
            templatePrint("                std::cout << element << \" \";");
            if (Configuration::logIt) {
                templatePrint("                log(element);");
            }
            templatePrint("            }");
            templatePrint("            std::cout << std::endl;");
            templatePrint("        }");
            if (Configuration::logIt) {
                templatePrint("        log(\"---  \" + std::string(\"" + fullName + "\"));");
            }
        }
        else {
            // Original handling for other types
            if (Configuration::logIt) {
                templatePrint("        log(\"---  \" + std::string(\"" + fullName + "\"));");
                templatePrint("        for (const auto& v : values) { log(v.to_string()); }");
            }
            std::string name = listElement + "Internal";
            templatePrint("        for (const " + listElement + "& value : values){");
            templatePrint("             std::cout << value.to_string() << std::endl;");
            templatePrint("             // Add calls to production code and asserts");
            if (listElement != "std::string" &&
                parent->dataNamesInternal.find(name) != parent->dataNamesInternal.end()) {
                templatePrint("              " + name + " i = value.to" + name + "();");
            }
            templatePrint("              }");
        }

        if (!Configuration::inTest) {
            templatePrint("        FAIL() << \"Must implement\";");
        }
        templatePrint("    }");
        templatePrint("");
    }
    void TemplateConstruct::makeFunctionTemplateNothing(const std::string& dataType, const std::string& fullName) {
        if (checkForExistingTemplate(dataType, fullName)) return;
        parent->glueFunctions[fullName] = dataType;
        templateHeaderPrint("   void " + fullName + "(); ");
        templatePrint("    void " + parent->glueClass + "::" + fullName + "(){");
        templatePrint("        std::cout << \"---  \" << \"" + fullName + "\" << std::endl;");
        if (Configuration::logIt) {
            templatePrint("        log(\"---  \" + std::string(\"" + fullName + "\"));");
        }
        if (!Configuration::inTest) {
            templatePrint("        FAIL() << \"Must implement\";");
        }
        templatePrint("    }");
        templatePrint("");
    }
    void TemplateConstruct::makeFunctionTemplateObject(const std::string& dataType, const std::string& fullName,
        const std::string& listElement) {
        if (checkForExistingTemplate(dataType, fullName)) return;
        std::cout << "Creating function template object for " << fullName << " with data type " << dataType << std::endl;   
        if (dataType != "std::vector<std::vector<std::string>>") {
            std::cerr << "Not creating function for " << fullName << " with data type " << dataType << std::endl;
            return;
        }
        parent->glueFunctions[fullName] = dataType;
        templateHeaderPrint("   void " + fullName + "(const " + dataType + "& values); ");
        templatePrint("    void " + parent->glueClass + "::" + fullName + "(const " + dataType + "& values ) {");
        if (Configuration::logIt) {
            templatePrint("        log(\"---  \" + std::string(\"" + fullName + "\"));");
            //templatePrint("    std::vector<std::vector<" + listElement + ">> is = convertListTo" + dataType + "(values);");
        templatePrint("    for (const auto& row : is) {");
        templatePrint("        for (const auto& element : row) {");
        templatePrint("            std::cout << element << \" \";");
        if (Configuration::logIt) {
            templatePrint("            log(element);");
        }
        templatePrint("        }");
        templatePrint("        std::cout << std::endl;");
        templatePrint("    }");
        }
        if (!Configuration::inTest) {
            templatePrint("        FAIL() << \"Must implement\";");
        }
        templatePrint("    }");
        templatePrint("");
    }
        int TemplateConstruct::processNamespaces(const std::string& packagePath) {
         std::vector<std::string> parts = parent->extractNamespaceParts(packagePath);

        for (const auto& part : parts) {
            templatePrint("namespace " + part + " {");
            templateHeaderPrint("namespace " + part + " {");
         }

        return static_cast<int>(parts.size());
    }
    void TemplateConstruct::endNamespace(const std::string& packagePath) {
        size_t count = std::count(packagePath.begin(), packagePath.end(), '.');

        for (size_t i = 0; i < count + 1; ++i) {
            templatePrint("}");
            templateHeaderPrint("}");
        }
    }

    void TemplateConstruct::beginTemplate() {
       
        for (const std::string& line : parent->linesToAddForDataAndGlue) {
            templatePrint(line);
        }

        if (Configuration::testFramework == "gtest") {
            templatePrint("#include <gtest/gtest.h>");
        }
        //   Change for other test frameworks if needed
        else if (Configuration::testFramework == "TestNG") {
            templatePrint("import org.testng.annotations.*;");
        }
        else {
            templatePrint("#include <gtest/gtest.h>");
        }
        templateHeaderPrint("#include <vector>");
        templateHeaderPrint("#include <string>");
        if (Configuration::logIt) {
            templatePrint("#include <fstream>");
            templatePrint("#include <iostream>"); 
            templatePrint("#include <filesystem>");
        }
        for (const std::string& line : parent->linesToAddForDataAndGlue) {
            templatePrint(line);
        }
        templatePrint("#include \"" + parent->glueClass + ".h\"");
        templatePrint("#include \"" + parent->dataHeaderFilename + "\"");

        templateHeaderPrint("#include \"" + parent->dataHeaderFilename + "\"");
        templatePrint("using namespace gherkinexecutor::" + parent->featureName + "; ");
        processNamespaces(parent->packagePath);

        templatePrint("");
        templateHeaderPrint("class " + parent->glueClass + " {");
        templateHeaderPrint("public:");
        templateHeaderPrint("    std::string DNCString;");
        templatePrint("std::string DNCString = \"" + Configuration::doNotCompare + "\";");
        templateHeaderPrint(parent->logIt());
        templateHeaderPrint("");
    }

    void TemplateConstruct::endTemplate() {
        for (const std::string& line : parent->linesToAddToEndOfGlue) {
            templatePrint(line);
        }

        templateHeaderPrint("    };"); // End the class
        endNamespace(parent->packagePath);
        try {
            if (parent->testFile) {
                parent->testFile->close();
            }
            if (glueTemplateFile) {
                glueTemplateFile->close();
            }
        }
        catch (const std::exception& e) {
            parent->error("Error in closing ");
        }
        try {
             if (glueTemplateFileHeader) {
                glueTemplateFileHeader->close();
            }
        }
        catch (const std::exception& e) {
            parent->error("Error in closing ");
        }

    }

} // namespace gherkinexecutor