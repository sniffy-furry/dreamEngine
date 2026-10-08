#pragma once
#include <string>
#include <vector>
namespace astra { struct AssemblyDefinition { std::string name; std::vector<std::string> references; std::vector<std::string> platforms; bool unsafe=false; }; }
