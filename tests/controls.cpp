#include <floks/controls.hpp>
#include <cassert>
#include <stdexcept>
using namespace floks;
template<class F>void rejects(F f){bool ok=false;try{f();}catch(const std::exception&){ok=true;}assert(ok);}
int main(){
  auto registry=default_registry();Tree t(registry);
  t.create("root","Panel");t.set_root("root");
  t.create("btn","Button");t.append("root","btn");
  t.create("toggle","Toggle");t.append("root","toggle");
  t.create("slider","Slider");t.append("root","slider");
  t.create("input","TextInput");t.append("root","input");
  Controls controls(t);int clicks=0,changes=0;
  t.on("btn","activate",[&](const Event&){++clicks;});
  t.on("toggle","change",[&](const Event& e){assert(std::get<bool>(e.payload));++changes;});
  t.on("slider","change",[&](const Event& e){assert(std::get<double>(e.payload)==0.5);++changes;});
  t.on("input","change",[&](const Event& e){assert(std::get<std::string>(e.payload)=="hello");++changes;});
  assert(controls.activate_button("btn"));assert(clicks==1);
  t.set_property("btn","enabled",false);assert(!controls.activate_button("btn"));
  auto rev=t.revision();assert(!controls.toggle("toggle",rev-1));assert(controls.toggle("toggle",rev));
  rev=t.revision();assert(controls.slider("slider",0.5,rev));
  rejects([&]{controls.slider("slider",2.0,t.revision());});
  rev=t.revision();assert(controls.text_input("input","hello",rev));
  assert(changes==3);
  rejects([&]{controls.activate_button("toggle");});
  return 0;
}
