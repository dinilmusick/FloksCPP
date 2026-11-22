#include <floks/serialization.hpp>
#include <cassert>
#include <stdexcept>
using namespace floks;
template<class F>void rejects(F f){bool ok=false;try{f();}catch(const std::exception&){ok=true;}assert(ok);}
int main(){
  Graph g;
  g.add_node({"a","Alpha",{10,15},{{"out","midi",Port::Direction::Output,0}}});
  g.add_node({"b","Beta",{20,20},{{"in","midi",Port::Direction::Input,0}}});
  g.connect({"wire","a","out","b","in"});g.select("a");g.set_viewport({{2,3},1.5});
  auto wire=serialize_graph(g);auto h=deserialize_graph(wire);
  assert(h.nodes().size()==2&&h.links().size()==1&&h.selection().count("a")&&h.viewport().zoom==1.5);
  assert(serialize_graph(h)==wire);
  rejects([&]{(void)deserialize_graph(wire+"bad");});
  rejects([&]{(void)deserialize_graph("FLOKS_GRAPH1 0 0 0 1 0 0\nN \"n\" \"x\" 1 2 0\n");});
  rejects([&]{(void)deserialize_graph("FLOKS_GRAPH1 0 0 1 1 0 0\nN \"n\" \"x\" 1 2 1\nP \"p\" \"midi\" 3 0\n");});
  PianoRoll r;r.insert({"a",60,1,2,110});r.add_marker({"a","verse",3});r.set_loop({1,5,true});r.seek(4);
  auto music=serialize_roll(r);auto rr=deserialize_roll(music);
  assert(rr.notes().size()==1&&rr.markers().size()==1&&rr.playhead()==4&&rr.loop().enabled);
  assert(serialize_roll(rr)==music);
  rejects([&]{(void)deserialize_roll(music+"junk");});
  rejects([&]{(void)deserialize_roll("FLOKS_ROLL1 0 0 1 0 1 0\nN \"bad\" 128 0 1 100\n");});
  return 0;
}
