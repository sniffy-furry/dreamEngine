#pragma once
#include <string>
#include <unordered_map>
#include <vector>
namespace astra {
struct PackageDependency { std::string name; std::string version; };
struct PackageManifest { std::string name; std::string version; std::string source; std::vector<PackageDependency> dependencies; };
class PackageManager {
public:
    void add(PackageManifest p){packages_[p.name]=std::move(p);} const PackageManifest* get(const std::string& n) const {auto it=packages_.find(n);return it==packages_.end()?nullptr:&it->second;} size_t count()const{return packages_.size();}
private: std::unordered_map<std::string,PackageManifest> packages_;
};
}
