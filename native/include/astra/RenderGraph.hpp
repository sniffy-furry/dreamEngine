#pragma once
#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>
namespace astra {
using RenderResourceId=std::uint32_t;
struct RenderResource { RenderResourceId id=0; std::string name; bool transient=true; };
struct RenderPass { std::string name; std::vector<RenderResourceId> reads,writes; std::function<void()> execute; };
class RenderGraph {
public:
    RenderResourceId createResource(std::string name,bool transient=true){auto id=++next_;resources_.push_back({id,std::move(name),transient});return id;}
    void addPass(RenderPass p){passes_.push_back(std::move(p));}
    std::vector<size_t> compile() const {
        const size_t n=passes_.size(); std::vector<int> indeg(n); std::vector<std::vector<size_t>> out(n);
        for(size_t i=0;i<n;i++)for(size_t j=i+1;j<n;j++){bool dep=false;for(auto w:passes_[i].writes) for(auto r:passes_[j].reads)dep|=(w==r);for(auto w:passes_[i].writes) for(auto w2:passes_[j].writes)dep|=(w==w2);if(dep){out[i].push_back(j);++indeg[j];}}
        std::vector<size_t> q;for(size_t i=0;i<n;i++)if(indeg[i]==0)q.push_back(i);for(size_t k=0;k<q.size();k++){auto u=q[k];for(auto v:out[u])if(--indeg[v]==0)q.push_back(v);}return q;
    }
    void execute(){for(auto i:compile())if(passes_[i].execute)passes_[i].execute();}
private:RenderResourceId next_=0;std::vector<RenderResource>resources_;std::vector<RenderPass>passes_;
};
}
