#pragma once
#include <string>
#include "Translate.h"
namespace gherkinexecutor {
    class DefineConstruct {
    public:
        class DefineData {
        public:
            std::string name;
            std::string value;

            DefineData(const std::string& name, const std::string& value) : name(name), value(value) {}

            std::string toString() const {
                return " name = " + name + " value = " + value;
            }
        };

    private:
        Translate* parent;

    public:
        explicit DefineConstruct(Translate* parent) : parent(parent) {}

        void actOnDefine(const std::vector<std::string>& words);

    private:
        void createDefineList(const std::vector<std::string>& table, std::vector<DefineData>& variables);
        void checkHeaders(const std::vector<std::string>& headers);
    };
}
