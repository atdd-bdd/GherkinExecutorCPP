#pragma once
#include "Translate.h"
namespace gherkinexecutor {
    class Translate;
    class TemplateConstruct {
    private:
        Translate* parent;

    public:
        std::unique_ptr<std::ofstream> glueTemplateFile;
        std::unique_ptr<std::ofstream> glueTemplateFileHeader;      
        int processNamespaces(const std::string& packagePath);
        void endNamespace(const std::string& packagePath);

        explicit TemplateConstruct(Translate* parent) : parent(parent) {}
        ~TemplateConstruct() = default;

        void beginTemplate();
        void endTemplate();
        void makeFunctionTemplate(const std::string& dataType, const std::string& fullName);
        void makeFunctionTemplateIsList(const std::string& dataType, const std::string& fullName,
            const std::string& listElement);
        void makeFunctionTemplateNothing(const std::string& dataType, const std::string& fullName);
        void makeFunctionTemplateObject(const std::string& dataType, const std::string& fullName,
            const std::string& listElement);

    private:
        void templatePrint(const std::string& line);
        bool checkForExistingTemplate(const std::string& dataType, const std::string& fullName);
    };
}


