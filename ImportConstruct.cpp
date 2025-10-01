#pragma once

#include <string>
#include <vector>
#include "ImportConstruct.h"
#include "DefineConstruct.h"
namespace gherkinexecutor {

    class Translate;

    // DefineConstruct class
  
    // ImportConstruct implementation
    void ImportConstruct::actOnImport(const std::vector<std::string>& words) {
        auto follow = parent->lookForFollow();
        std::string followType = follow.first;
        std::vector<std::string> table = follow.second;

        if (followType != "TABLE") {
            parent->error("Error table does not follow import " + words[0] + " " + words[1]);
            return;
        }

        std::vector<ImportData> imports;
        createImportList(table, imports);

        for (const ImportData& im : imports) {
            if (parent->importNames.find(im.dataType) != parent->importNames.end()) {
                parent->error("Data type is duplicated, has been renamed " + im.dataType);
                continue;
            }

            if (!im.conversionMethod.empty()) {
                parent->importNames[im.dataType] = im.conversionMethod;
            }
            else {
                std::string methodName = im.dataType + "($)";
                parent->importNames[im.dataType] = methodName;
            }
        }

        for (const ImportData& im : imports) {
            if (!im.importName.empty()) {
                std::string value = "import " + im.importName + ";";
                parent->linesToAddForDataAndGlue.push_back(value);
            }
        }
    }

    void ImportConstruct::createImportList(const std::vector<std::string>& table, std::vector<ImportData>& variables) {
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
                variables.emplace_back(elements[0], elements[1], elements[2], elements[3]);
            }
            else if (elements.size() > 2) {
                variables.emplace_back(elements[0], elements[1], elements[2]);
            }
            else {
                variables.emplace_back(elements[0], elements[1]);
            }
        }
    }

    void ImportConstruct::checkHeaders(const std::vector<std::string>& headers) {
        std::vector<std::string> expected = { "Datatype", "ConversionMethod", "Import", "Notes" };
        if (headers.size() < 2 || headers[0] != expected[0] || headers[1] != expected[1]) {
            parent->error("Headers should start with " + expected[0] + ", " + expected[1]);
        }
    }

    // DefineConstruct implementation
    void DefineConstruct::actOnDefine(const std::vector<std::string>& words) {
        auto follow = parent->lookForFollow();
        std::string followType = follow.first;
        std::vector<std::string> table = follow.second;

        if (followType != "TABLE") {
            parent->error("Error table does not follow define " + words[0] + " " + words[1]);
            return;
        }

        std::vector<DefineData> defines;
        createDefineList(table, defines);

        for (const DefineData& im : defines) {
            if (parent->defineNames.find(im.name) != parent->defineNames.end()) {
                parent->warning("Define is duplicated will be skipped " + im.name + " = " + im.value);
                continue;
            }

            if (im.value.empty()) {
                parent->warning("Empty value for define ");
            }

            parent->defineNames[im.name] = im.value;
        }
    }

    void DefineConstruct::createDefineList(const std::vector<std::string>& table, std::vector<DefineData>& variables) {
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
            }
            else {
                variables.emplace_back(elements[0], elements[1]);
            }
        }
    }

    void DefineConstruct::checkHeaders(const std::vector<std::string>& headers) {
        std::vector<std::string> expected = { "Name", "Value", "Notes" };
        if (headers.size() < 2 || headers[0] != expected[0] || headers[1] != expected[1]) {
            parent->error("Headers should start with " + expected[0] + ", " + expected[1]);
        }
    }

} // namespace gherkinexecutor