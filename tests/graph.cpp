#include <floks/graph.hpp>
#include <cassert>
#include <cmath>
#include <stdexcept>
using namespace floks;
template<class F>void rejects(F f){bool ok=false;try{f();}catch(const std::exception&){ok=true;}assert(ok);}
int main(){
  Graph g;
  g.add_node({"A","Source",{0,0},{{"o","midi",Port::Direction::Output,1},{"i","midi",Port::Direction::Input,1}}});
  g.add_node({"B","Sink",{10,20},{{"i","midi",Port::Direction::Input,1},{"o","midi",Port::Direction::Output,1}}});
  g.add_node({"C","Other",{0,10},{{"i","osc",Port::Direction::Input,1}}});
  rejects([&]{g.add_node({"A","Duplicate",{},{}});});
  rejects([&]{g.add_node({"D","bad",{},{{"x","midi",Port::Direction::Input,0},{"x","midi",Port::Direction::Input,0}}});});
  assert(!g.can_connect("A","o","C","i"));
  assert(!g.can_connect("A","i","B","i"));
  assert(!g.can_connect("A","o","A","i"));
  assert(g.can_connect("A","o","B","i"));
  g.connect({"l1","A","o","B","i"});
  rejects([&]{g.connect({"l2","A","o","B","i"});});
  assert(!g.can_connect("B","o","A","i")); // DAG cycle
  g.select("B");assert(g.selection().count("B"));
  g.move_node("B",{20,30});assert(g.nodes().at("B").position.x==20);
  g.set_viewport({{25,-10},2.0});
  auto p=g.world_to_screen({3,4});assert(p.x==31&&p.y==-2);
  auto q=g.screen_to_world(p);assert(q.x==3&&q.y==4);
  rejects([&]{g.set_viewport({{},0});});
  assert(g.undo());assert(g.viewport().zoom==1);
  assert(g.redo());assert(g.viewport().zoom==2);
  g.remove_node("B");assert(g.links().empty());
  assert(g.undo());assert(g.links().size()==1);
  return 0;
}
