#pragma once

#include <string>
#include <vector>
#include <unordered_set>
#include <sstream>
#include <algorithm>
#include "TagFilterEvaluator.h"
namespace gherkinexecutor {

  
    // Implementation
    bool TagFilterEvaluator::shouldNotExecute(const std::vector<std::string>& words, const std::string& filterExpression) {
        std::unordered_set<std::string> scenarioTags(words.begin(), words.end());
        return !shouldExecute(scenarioTags, filterExpression);
    }

    bool TagFilterEvaluator::shouldExecute(const std::unordered_set<std::string>& scenarioTags, const std::string& filterExpression) {
        std::string trimmedExpr = filterExpression;
        // Trim whitespace
        trimmedExpr.erase(0, trimmedExpr.find_first_not_of(" \t"));
        trimmedExpr.erase(trimmedExpr.find_last_not_of(" \t") + 1);

        if (trimmedExpr.empty()) return true;

        std::vector<std::unordered_set<std::string>> requiredConditions;
        std::unordered_set<std::string> excludedTags;

        parseExpression(filterExpression, requiredConditions, excludedTags);

        // Check if scenario contains any excluded tags
        bool hasExcludedTag = std::any_of(scenarioTags.begin(), scenarioTags.end(),
            [&excludedTags](const std::string& tag) {
                return excludedTags.find(tag) != excludedTags.end();
            });

        // Check if scenario matches any required condition group (OR logic)
        bool matchesRequired = requiredConditions.empty() ||
            std::any_of(requiredConditions.begin(), requiredConditions.end(),
                [&scenarioTags](const std::unordered_set<std::string>& condition) {
                    return std::all_of(condition.begin(), condition.end(),
                        [&scenarioTags](const std::string& tag) {
                            return scenarioTags.find(tag) != scenarioTags.end();
                        });
                });

        // Execute if it meets a required condition AND does NOT have an excluded tag
        return matchesRequired && !hasExcludedTag;
    }

    void TagFilterEvaluator::parseExpression(const std::string& expression,
        std::vector<std::unordered_set<std::string>>& requiredConditions,
        std::unordered_set<std::string>& excludedTags) {
        // Split by " OR "
        std::vector<std::string> groups;
        std::string remaining = expression;
        size_t pos = 0;

        while ((pos = remaining.find(" OR ")) != std::string::npos) {
            groups.push_back(remaining.substr(0, pos));
            remaining = remaining.substr(pos + 4); // length of " OR "
        }
        groups.push_back(remaining);

        for (const std::string& groupStr : groups) {
            std::unordered_set<std::string> tags;

            // Split by " AND "
            std::vector<std::string> elements;
            std::string remainingGroup = groupStr;
            pos = 0;

            while ((pos = remainingGroup.find(" AND ")) != std::string::npos) {
                elements.push_back(remainingGroup.substr(0, pos));
                remainingGroup = remainingGroup.substr(pos + 5); // length of " AND "
            }
            elements.push_back(remainingGroup);

            for (std::string element : elements) {
                // Trim element
                element.erase(0, element.find_first_not_of(" \t"));
                element.erase(element.find_last_not_of(" \t") + 1);

                if (element.find("NOT ") == 0) {
                    std::string excludedTag = element.substr(4); // Remove "NOT "
                    excludedTag.erase(0, excludedTag.find_first_not_of(" \t"));
                    excludedTag.erase(excludedTag.find_last_not_of(" \t") + 1);
                    excludedTags.insert(excludedTag);
                }
                else {
                    tags.insert(element);
                }
            }

            if (!tags.empty()) {
                requiredConditions.push_back(tags);
            }
        }
    }

} // namespace gherkinexecutor