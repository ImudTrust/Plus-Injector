#pragma once

#include <string>
#include <vector>
#include <unordered_map>

namespace pi {

    class CommandLineArguments {
    public:
        explicit CommandLineArguments(int argc, char** argv);

        bool IsSwitchPresent(const std::string& name) const;
        bool HasValue(const std::string& name) const;
        bool GetStringArg(const std::string& name, std::string& out) const;
        bool GetIntArg(const std::string& name, int& out) const;
        bool GetLongArg(const std::string& name, long long& out) const;
        std::vector<std::string> GetUnknownSwitches(
            const std::vector<std::string>& allowed) const;
        std::vector<std::string> GetDuplicateSwitches() const;

    private:
        bool GetArgValue(const std::string& name, std::string& out) const;

        std::vector<std::string> _tokens;
        std::unordered_map<std::string, std::string> _values;
        std::vector<std::string> _switches;
        std::vector<std::string> _duplicates;
    };

}
