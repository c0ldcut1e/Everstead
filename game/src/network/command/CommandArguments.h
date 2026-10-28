#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

class CommandArguments
{
public:
    CommandArguments();
    ~CommandArguments();

    bool Parse(const std::string &line, nlohmann::json *errorMessage);

    size_t Count() const;
    const std::string &Get(size_t index) const;
    bool GetInt(size_t index, int minimum, int maximum, int *value, nlohmann::json *errorMessage) const;
    bool GetDouble(size_t index, double *value, nlohmann::json *errorMessage) const;
    std::string GetRest(size_t index) const;
    std::string JoinFrom(size_t index, const std::string &delimiter) const;
    const std::vector<std::string> &GetValues() const;

private:
    void Clear();
    bool ReadQuoted(size_t *index, std::string *value, nlohmann::json *errorMessage) const;
    bool ReadBare(size_t *index, std::string *value) const;
    void SkipWhitespace(size_t *index) const;

    std::string m_line;
    std::vector<std::string> m_values;
    std::vector<size_t> m_starts;
};
