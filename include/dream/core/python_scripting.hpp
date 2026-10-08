#pragma once
#include <string>
#include <vector>
namespace dream {
class PythonScriptManager final {
public:
    bool scan(const std::string& directory);
    const std::vector<std::string>& scripts() const noexcept { return scripts_; }
private:
    std::vector<std::string> scripts_;
};
}
