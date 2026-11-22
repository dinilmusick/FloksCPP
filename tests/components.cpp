#include <floks/components.hpp>
#include <cassert>
#include <memory>
#include <stdexcept>
using namespace floks;
template<class F> void rejects(F f){bool threw=false;try{f();}catch(const std::exception&){threw=true;}assert(threw);}
struct Capture:Renderer {std::size_t count=0;std::uint64_t last=0;void present(const Snapshot& root,std::uint64_t rev)override{assert(root.id=="root");++count;last=rev;}};
int main(){
  auto registry=default_registry();
  rejects([&]{registry.register_component({"Panel",{}, {},""});});
  Tree tree(registry);
  auto root=tree.create("root","Panel"); tree.set_root("root");
  auto button=tree.create("button","Button",{{"label",std::string("Go")}});
  tree.append("root","button");
  assert(root->mounted()&&button->mounted()&&button->child_count()==0);
  rejects([&]{tree.create("button","Button");});
  rejects([&]{tree.create("wrong","Button",{{"label",true}});});
  rejects([&]{tree.append("root","root");});
  rejects([&]{tree.append("button","root");});
  rejects([&]{tree.append("root","button");});
  tree.set_theme_token("root","accent",std::string("red"));
  assert(std::get<std::string>(tree.theme_value("button","accent"))=="red");
  tree.set_layout("button",{Flow::Row,4,{10,10,50,20}});
  rejects([&]{tree.set_layout("root",{Flow::Column,-1,{}});});
  tree.set_property("button","enabled",false);
  rejects([&]{tree.set_property("button","enabled",std::string("false"));});
  auto r=tree.revision();
  assert(!tree.execute({"button","enabled",true,r-1}));
  assert(tree.execute({"button","enabled",true,r}));
  int clicks=0;
  auto first=tree.on("button","activate",[&](const Event& e){assert(e.name=="activate");++clicks;rejects([&]{tree.dispatch("button","activate",true);});});
  tree.on("button","activate",[&](const Event&){++clicks;tree.off("button","activate",first);});
  tree.dispatch("button","activate",true);assert(clicks==2);
  tree.dispatch("button","activate",true);assert(clicks==3);
  int scoped=0;
  {
    auto guard=tree.on_scoped("button","activate",[&](const Event&){++scoped;});
    tree.dispatch("button","activate",true);
    assert(scoped==1);
  }
  tree.dispatch("button","activate",true);
  assert(scoped==1);
  rejects([&]{tree.dispatch("root","activate",true);});
  Capture renderer; present(tree,renderer); assert(renderer.count==1 && renderer.last==tree.revision());
  const auto wire=tree.serialize(); auto restored=Tree::deserialize(registry,wire);
  assert(restored.size()==2 && restored.require("button")->mounted());
  assert(restored.require("button")->layout().bounds.width==50);
  assert(std::get<std::string>(restored.theme_value("button","accent"))=="red");
  assert(std::get<std::string>(restored.require("button")->props().at("label"))=="Go");
  rejects([&]{(void)Tree::deserialize(registry,"FAKE\n");});
  rejects([&]{(void)Tree::deserialize(registry,wire+"junk");});
  tree.remove("root"); assert(!button->mounted()&&!root->mounted()&&tree.size()==0);
  rejects([&]{tree.dispatch("button","activate",true);});
  return 0;
}
