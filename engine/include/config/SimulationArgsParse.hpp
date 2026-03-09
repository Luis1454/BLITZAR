#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

class SimulationArgsParse final {
  public:
    static std::string toLower(std::string value);
    static bool parseBool(const std::string &value, bool &out);
    static bool parseUint(const std::string &value, std::uint32_t &out);
    static bool parseInt(const std::string &value, int &out);
    static bool parseFloat(const std::string &value, float &out);
    static bool splitOption(const std::string &raw, std::string &key, std::string &value);
    static bool readValue(
        const std::vector<std::string_view> &args,
        std::size_t &index,
        const std::string &inlined,
        std::string &outValue
    );
};

