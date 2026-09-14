#include "header/Args.hpp"
#include <cstdlib>
#include <algorithm>

namespace pi {

    CommandLineArguments::CommandLineArguments(int argc, char** argv) {
        for (int i = 0; i < argc; ++i) {
            _tokens.emplace_back(argv[i]);
        }

        for (size_t i = 1; i < _tokens.size(); ++i) {
            const std::string& t = _tokens[i];
            if (!t.empty() && t[0] == '-') {
                std::string name = t;
                std::string value;
                const size_t equals = t.find('=');
                if (equals != std::string::npos) {
                    name = t.substr(0, equals);
                    value = t.substr(equals + 1);
                }
                else if (i + 1 < _tokens.size() &&
                    (_tokens[i + 1].empty() || _tokens[i + 1][0] != '-')) {
                    value = _tokens[++i];
                }

                if (_values.find(name) != _values.end()) {
                    _duplicates.push_back(name);
                }
                _switches.push_back(name);
                _values[name] = value;
            }
        }
    }

    bool CommandLineArguments::IsSwitchPresent(const std::string& name) const {
        return _values.find(name) != _values.end();
    }

    bool CommandLineArguments::HasValue(const std::string& name) const {
        auto it = _values.find(name);
        return it != _values.end() && !it->second.empty();
    }

    bool CommandLineArguments::GetArgValue(const std::string& name,
        std::string& out) const {
        auto it = _values.find(name);
        if (it == _values.end() || it->second.empty()) return false;
        out = it->second;
        return true;
    }

    bool CommandLineArguments::GetStringArg(const std::string& name,
        std::string& out) const {
        return GetArgValue(name, out);
    }

    bool CommandLineArguments::GetIntArg(const std::string& name,
        int& out) const {
        std::string s;
        if (!GetArgValue(name, s)) return false;
        char* end = nullptr;
        long v = std::strtol(s.c_str(), &end, 10);
        if (end == s.c_str() || *end != '\0') return false;
        out = (int)v;
        return true;
    }

    bool CommandLineArguments::GetLongArg(const std::string& name,
        long long& out) const {
        std::string s;
        if (!GetArgValue(name, s)) return false;
        char* end = nullptr;
        long long v = std::strtoll(s.c_str(), &end, 0);
        if (end == s.c_str() || *end != '\0') return false;
        out = v;
        return true;
    }

    std::vector<std::string> CommandLineArguments::GetUnknownSwitches(
        const std::vector<std::string>& allowed) const {
        std::vector<std::string> unknown;
        for (const auto& option : _switches) {
            if (std::find(allowed.begin(), allowed.end(), option) == allowed.end()) {
                unknown.push_back(option);
            }
        }
        return unknown;
    }

    std::vector<std::string> CommandLineArguments::GetDuplicateSwitches() const {
        return _duplicates;
    }

}
