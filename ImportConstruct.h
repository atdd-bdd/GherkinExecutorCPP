#pragma once
#include <string>

// ImportConstruct class
#include "ImportConstruct.h"
#include "Translate.h"
namespace gherkinexecutor {
    class Translate; 
    class ImportConstruct {
    public:
        class ImportData {
        public:
            std::string dataType;
            std::string importName;
            std::string conversionMethod;
            std::string notes;

            ImportData(const std::string& dataType, const std::string& conversionMethod,
                const std::string& importName, const std::string& notes)
                : dataType(dataType), importName(importName), conversionMethod(conversionMethod), notes(notes) {
            }
            ImportData(const std::string& dataType, const std::string& conversionMethod, const std::string& importName)
                : ImportData(dataType, conversionMethod, importName, "") {
            }
            ImportData(const std::string& dataType, const std::string& conversionMethod)
                : ImportData(dataType, conversionMethod, "", "") {
            }
        };

    private:
        Translate* parent;

    public:
        explicit ImportConstruct(Translate* parent) : parent(parent) {}

        void actOnImport(const std::vector<std::string>& words);

    private:
        void createImportList(const std::vector<std::string>& table, std::vector<ImportData>& variables);
        void checkHeaders(const std::vector<std::string>& headers);
    };

}
