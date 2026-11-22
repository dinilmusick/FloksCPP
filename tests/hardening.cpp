#include <floks/floks.hpp>
#include <floks/serialization.hpp>
#include <cassert>
#include <limits>
#include <memory>
#include <cmath>
using namespace floks;
template <typename Fn> void rejects(Fn fn) {bool raised=false;try{fn();}catch(const std::exception&){raised=true;}assert(raised);}
int main() {
  auto reg=default_registry();
  Tree t(reg);t.create("root","Panel");t.set_root("root");t.create("btn","Button");t.append("root","btn");
  assert(reg.require("Select").events.count("change"));
  assert(reg.require("TextArea").events.count("submit"));
  rejects([&]{t.create("bad","Slider",{{"value",std::numeric_limits<double>::infinity()}});});
  rejects([&]{t.set_theme_token("btn","error",std::numeric_limits<double>::quiet_NaN());});
  int n=0;t.on("btn","activate",[&](const Event&){++n; rejects([&]{t.remove("btn");}); rejects([&]{t.create("new","Text");});});
  t.dispatch("btn","activate",true);assert(n==1&&t.size()==2);
  PianoRoll r;r.add_marker({"mark","Intro",1});r.move_marker("mark",2);assert(r.markers().at("mark").beat==2);r.rename_marker("mark","Verse");assert(r.markers().at("mark").label=="Verse");r.undo();assert(r.markers().at("mark").label=="Intro");
  rejects([&]{r.move_marker("mark",-1);});
  assert(r.y_to_pitch(-std::numeric_limits<double>::max())==127);
  r.seek(std::numeric_limits<double>::max());rejects([&]{r.advance(std::numeric_limits<double>::max());});

  return 0;
}
