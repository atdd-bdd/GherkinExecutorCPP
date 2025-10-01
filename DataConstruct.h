#pragma once
#include <string>
#include <vector>
#include <list> 
#include <memory>
#include <fstream>

#include "Configuration.h"
namespace gherkinexecutor {
    class Translate; 
    class DataConstruct {
    public:
        class DataValues {
        public:
            std::string name;
            std::string defaultVal;
            std::string dataType;
            std::string notes;

            DataValues(const std::string& name, const std::string& defaultVal,
                const std::string& dataType, const std::string& notes)
                : name(name), defaultVal(defaultVal), dataType(dataType), notes(notes) {
            }
            DataValues(const std::string& name, const std::string& defaultVal, const std::string& dataType)
                : DataValues(name, defaultVal, dataType, "") {
            }
            DataValues(const std::string& name, const std::string& defaultVal)
                : DataValues(name, defaultVal, "String", "") {
            }
        };

    private:
        Translate* parent;
        std::unique_ptr<std::ofstream> dataDefinitionFile;
        std::unique_ptr<std::ofstream> dataDefinitionFileHeader;
        static const std::string throwString;
        static bool oneDataFileStarted;

    public:
        explicit DataConstruct(Translate* parent) : parent(parent) {}

        void actOnData(const std::vector<std::string>& words);
        void endOneDataFile();

    private:
        int processNamespaces(const std::string& packagePath);
        void endNamespace(const std::string& packagePath);

        void createVariableList(const std::vector<std::string>& table, std::vector<DataValues>& variables, bool& doInternal);
        void createConstructor(const std::vector<DataValues>& variables, const std::string& className);
        void createInternalConstructor(const std::vector<DataValues>& variables, const std::string& className);
        void createEqualsMethod(const std::vector<DataValues>& variables, const std::string& className);
        void createInternalEqualsMethod(const std::vector<DataValues>& variables, const std::string& className);
        void createBuilderMethod(const std::vector<DataValues>& variables, const std::string& className);
        void createToStringMethod(const std::vector<DataValues>& variables, const std::string& className);
        void createToStringMethodInternal(const std::vector<DataValues>& variables, const std::string& className);
        void createToJSONMethod(const std::vector<DataValues>& variables, const std::string& className);
        void createFromJSONMethod(const std::vector<DataValues>& variables, const std::string& className);
        void createTableToJSONMethod(const std::string& className);
        void createJSONToTableMethod(const std::string& className);
        void createMergeMethod(const std::vector<DataValues>& variables, const std::string& className);
        void createConversionMethod(const std::string& internalClassName, const std::vector<DataValues>& variables, const std::string& className);
        void createInternalClass(const std::string& className, const std::string& otherClassName,
            const std::vector<DataValues>& variables, bool providedClassName);
        void createDataTypeToStringObject(const std::string& className, const std::vector<DataValues>& variables);
        void createToStringObject(const std::string& className, const std::string& otherClassName, const std::vector<DataValues>& variables);
        std::string makeValueToString(const DataValues& variable, bool makeNameValue);
        bool primitiveDataType(const DataValues& variable);
        std::string alterDataType(const std::string& s);
        void checkHeaders(const std::vector<std::string>& headers);
        void startDataFile(const std::string& className, bool createTmpl);
        void startOneDataFile();
        void endDataFile();
        void addSuppressionOfWarnings();
        void dataPrintLn(const std::string& line);
        void dataPrintHeader(const std::string& line);
    };
}


