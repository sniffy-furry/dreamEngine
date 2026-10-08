#include "dream/core/python_scripting.hpp"
#include <dirent.h>
#include <cstring>
#include <algorithm>
namespace dream {
bool PythonScriptManager::scan(const std::string& directory) {
    scripts_.clear(); DIR* d=opendir(directory.c_str()); if(!d) return false;
    while(auto* e=readdir(d)){ const char* n=e->d_name; size_t l=std::strlen(n); if(l>3 && std::strcmp(n+l-3,".py")==0) scripts_.push_back(directory+"/"+n); }
    closedir(d); std::sort(scripts_.begin(),scripts_.end()); return true;
}
}
