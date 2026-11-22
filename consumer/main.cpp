#include <floks/floks.hpp>
#include <cassert>
#ifdef FLOKSCPP_CONSUMER_FRNL
#include <floks/frnl_bridge.hpp>
#endif
int main(){
  auto registry=floks::default_registry();
  floks::Tree tree(registry);
  tree.create("root","Panel");tree.set_root("root");
  floks::Graph graph;
  graph.add_node({"one","test",{}, {}});
  floks::PianoRoll roll;
  roll.insert({"n",60,0,1,90});
  assert(tree.size()==1&&graph.nodes().size()==1&&roll.notes().size()==1);
#ifdef FLOKSCPP_CONSUMER_FRNL
  frnl::SemanticRuntime runtime;
  floks::FrnlBridge bridge(runtime);
  bool fired=false;
  bridge.register_button("btn",[&]{fired=true;});
  bridge.activate("btn");
  assert(fired);
#endif
}
