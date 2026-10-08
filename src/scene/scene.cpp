#include "dream/scene/scene.hpp"
#include <fstream>
#include <sstream>
#include <iomanip>
namespace dream::scene {
static void esc(std::ostream&o,const std::string&s){o<<'"';for(char c:s){if(c=='"'||c=='\\')o<<'\\';o<<c;}o<<'"';}
std::string serialize(const Scene&s){std::ostringstream o;o<<"{\n  \"format\":1,\n  \"name\":";esc(o,s.name);o<<",\n  \"entities\":[\n";for(size_t n=0;n<s.world.entities().size();++n){auto e=s.world.entities()[n];o<<"    {\"index\":"<<e.index<<",\"generation\":"<<e.generation; if(auto*t=s.world.transform(e))o<<",\"transform\":["<<t->px<<","<<t->py<<","<<t->pz<<","<<t->rx<<","<<t->ry<<","<<t->rz<<","<<t->sx<<","<<t->sy<<","<<t->sz<<"]";if(auto*c=s.world.camera(e))o<<",\"camera\":["<<c->fov<<","<<c->near_plane<<","<<c->far_plane<<"]";if(auto*m=s.world.mesh(e))o<<",\"mesh\":["<<m->mesh<<","<<m->material<<"]";if(auto*l=s.world.light(e))o<<",\"light\":["<<l->r<<","<<l->g<<","<<l->b<<","<<l->intensity<<","<<l->range<<"]";o<<"}"<<(n+1<s.world.entities().size()?",":"")<<"\n";}o<<"  ]\n}\n";return o.str();}
static bool num(const std::string&s,size_t& p,double&v){while(p<s.size()&&(s[p]==' '||s[p]=='\n'||s[p]=='\r'||s[p]=='\t'||s[p]==','||s[p]==':'))++p;size_t b=p;while(p<s.size()&&(s[p]=='-'||s[p]=='+'||s[p]=='.'||(s[p]>='0'&&s[p]<='9')||s[p]=='e'||s[p]=='E'))++p;if(b==p)return false;try{v=std::stod(s.substr(b,p-b));return true;}catch(...){return false;}}
static bool arr(const std::string&s,size_t p,const char* key,std::vector<double>&out){auto k=s.find(key,p);if(k==std::string::npos)return false;k=s.find('[',k);if(k==std::string::npos)return false;++k;for(int i=0;i<16&&k<s.size();++i){double v;if(!num(s,k,v))break;out.push_back(v);auto q=s.find_first_of("]",k);auto comma=s.find(',',k);if(q!=std::string::npos&&(comma==std::string::npos||q<comma))break;}return true;}
bool deserialize(Scene&out,const std::string&s,std::string*err){
 if(s.find("\"format\":1")==std::string::npos){if(err)*err="unsupported scene format";return false;}
 Scene temp;
 auto nk=s.find("\"name\":"); if(nk!=std::string::npos){auto a=s.find('"',nk+7);auto b=s.find('"',a+1);if(a!=std::string::npos&&b!=std::string::npos)temp.name=s.substr(a+1,b-a-1);}
 auto ep=s.find("\"entities\""); if(ep==std::string::npos){out=std::move(temp);return true;}
 size_t p=s.find('{',ep); while(p!=std::string::npos){
   size_t end=s.find('}',p); if(end==std::string::npos) break; std::string obj=s.substr(p,end-p+1);
   auto ik=obj.find("\"index\""); if(ik==std::string::npos) break;
   auto colon=obj.find(':',ik); size_t q=colon+1; double dummy=0; if(!num(obj,q,dummy)) break;
   auto e=temp.world.create(); std::vector<double> v;
   if(arr(obj,0,"\"transform\"",v)&&v.size()>=9){auto&t=temp.world.add_transform(e);t.px=v[0];t.py=v[1];t.pz=v[2];t.rx=v[3];t.ry=v[4];t.rz=v[5];t.sx=v[6];t.sy=v[7];t.sz=v[8];}
   v.clear(); if(arr(obj,0,"\"camera\"",v)&&v.size()>=3){auto&c=temp.world.add_camera(e);c.fov=v[0];c.near_plane=v[1];c.far_plane=v[2];}
   v.clear(); if(arr(obj,0,"\"mesh\"",v)&&v.size()>=2){auto&m=temp.world.add_mesh(e);m.mesh=(uint64_t)v[0];m.material=(uint64_t)v[1];}
   v.clear(); if(arr(obj,0,"\"light\"",v)&&v.size()>=5){auto&l=temp.world.add_light(e);l.r=v[0];l.g=v[1];l.b=v[2];l.intensity=v[3];l.range=v[4];}
   p=s.find("{\"index\":",end+1);
 }
 out=std::move(temp);return true;
}
bool save(const Scene&s,const std::string&p,std::string*e){std::ofstream f(p,std::ios::binary);if(!f){if(e)*e="cannot open scene for writing";return false;}f<<serialize(s);return (bool)f;}
bool load(Scene&s,const std::string&p,std::string*e){std::ifstream f(p,std::ios::binary);if(!f){if(e)*e="cannot open scene";return false;}std::ostringstream b;b<<f.rdbuf();return deserialize(s,b.str(),e);}
}
