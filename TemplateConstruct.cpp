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

        parent->glueFunctions[fullName] = dataType;
        templatePrint("    void " + fullName + "(" + dataType + " value ) {");
        templatePrint("        System.out.println(\"---  \" + \"" + fullName + "\");");

        if (Configuration::logIt) {
            templatePrint("        log(\"---  \" + \"" + fullName + "\");");
            templatePrint("        log(value.toString());");
        }

        templatePrint("        System.out.println(value);");

        if (!Configuration::inTest) {
            templatePrint("        fail(\"Must implement\");");
        }

        templatePrint("    }");
        templatePrint("");
    }

    void TemplateConstruct::makeFunctionTemplateIsList(const std::string& dataType, const std::string& fullName,
        const std::string& listElement) {
        if (checkForExistingTemplate(dataType, fullName)) return;

        parent->glueFunctions[fullName] = dataType;
        templatePrint("    void " + fullName + "(" + dataType + " values ) {");
        templatePrint("        System.out.println(\"---  \" + \"" + fullName + "\");");

        if (Configuration::logIt) {
            templatePrint("        log(\"---  \" + \"" + fullName + "\");");
            templatePrint("        log(values.toString());");
        }

        std::string name = listElement + "Internal";
        templatePrint("        for (" + listElement + " value : values){");
        templatePrint("             System.out.println(value);");
        templatePrint("             // Add calls to production code and asserts");

        if (dataType != "List<List<String>>" && listElement != "String" &&
            parent->dataNamesInternal.find(name) != parent->dataNamesInternal.end()) {
            templatePrint("              " + name + " i = value.to" + name + "();");
        }

        templatePrint("              }");

        if (!Configuration::inTest) {
            templatePrint("        fail(\"Must implement\");");
        }

        templatePrint("    }");
        templatePrint("");
    }

    void TemplateConstruct::makeFunctionTemplateNothing(const std::string& dataType, const std::string& fullName) {
        if (checkForExistingTemplate(dataType, fullName)) return;

        parent->glueFunctions[fullName] = dataType;
        templatePrint("    void " + fullName + "(){");
        templatePrint("        System.out.println(\"---  \" + \"" + fullName + "\");");

        if (Configuration::logIt) {
            templatePrint("        log(\"---  \" + \"" + fullName + "\");");
        }

        if (!Configuration::inTest) {
            templatePrint("        fail(\"Must implement\");");
        }

        templatePrint("    }");
        templatePrint("");
    }

    void TemplateConstruct::makeFunctionTemplateObject(const std::string& dataType, const std::string& fullName,
        const std::string& listElement) {
        if (checkForExistingTemplate(dataType, fullName)) return;

        parent->glueFunctions[fullName] = dataType;
        templatePrint("    void " + fullName + "(" + dataType + " values ) {");
        templatePrint("    List<List<" + listElement + ">> is = convertListTo" + dataType + "(values);");
        templatePrint("    System.out.println(is);");

        if (Configuration::logIt) {
            templatePrint("        log(\"---  \" + \"" + fullName + "\");");
        }

        if (!Configuration::inTest) {
            templatePrint("        fail(\"Must implement\");");
        }

        templatePrint("    }");
        templatePrint("");
    }
    int TemplateConstruct::processNamespaces(const std::string& packagePath) {
         std::vector<std::string> parts = parent->extractNamespaceParts(packagePath);

        for (const auto& part : parts) {
            templatePrint("namespace " + part + " {");
         }

        return static_cast<int>(parts.size());
    }
    void TemplateConstruct::endNamespace(const std::string& packagePath) {
        size_t count = std::count(packagePath.begin(), packagePath.end(), '.');

        for (size_t i = 0; i < count + 1; ++i) {
            templatePrint("}");
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
        templatePrint("#include <vector>");
        templatePrint("#include <string>");
        if (Configuration::logIt) {
            templatePrint("#include <fstream>");
            templatePrint("#include <iostream>"); 
            templatePrint("#include <filesystem>");
        }
        templatePrint("using namespace gherkinexecutor::" + parent->packagePath + ";"); 
        processNamespaces(parent->packagePath);

        templatePrint("");
        templatePrint("class " + parent->glueClass + " {");
        templatePrint("    final string DNCString = \"" + Configuration::doNotCompare + "\";");
        templatePrint(parent->logIt());
        templatePrint("");
    }

    void TemplateConstruct::endTemplate() {
        for (const std::string& line : parent->linesToAddToEndOfGlue) {
            templatePrint(line);
        }

        templatePrint("    }"); // End the class
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
    }

} // namespace gherkinexecutor