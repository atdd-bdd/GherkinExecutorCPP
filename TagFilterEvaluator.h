#pragma once
#include <vector>
#include <string>
#include <unordered_set>
namespace gherkinexecutor {
    class TagFilterEvaluator {
    public:
        static bool shouldNotExecute(const std::vector<std::string>& words, const std::string& filterExpression);
        static bool shouldExecute(const std::unordered_set<std::string>& scenarioTags, const std::string& filterExpression);

    private:
        static void parseExpression(const std::string& expression,
            std::vector<std::unordered_set<std::string>>& requiredConditions,
            std::unordered_set<std::string>& excludedTags);
    };
}

