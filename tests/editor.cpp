#include <floks/floks.hpp>
#include <floks/controls.hpp>
#include <floks/serialization.hpp>
#include <cassert>
#include <cmath>
#include <stdexcept>
using namespace floks;
template<class F>void rejects(F f){bool threw=false;try{f();}catch(const std::exception&){threw=true;}assert(threw);}
int main() {
  Graph g;
  g.add_node({"a","source",{1.2,2.1},{{"out","midi",Port::Direction::Output,0}}});
  g.add_node({"b","target",{3.2,4.1},{{"in","midi",Port::Direction::Input,0}}});
  g.add_node({"c","target",{10,12},{{"in","midi",Port::Direction::Input,0}}});
  g.select("a");g.select("b");g.move_selection({1,2});
  assert(std::abs(g.nodes().at("a").position.x-2.2)<1.e-9&&std::abs(g.nodes().at("b").position.y-6.1)<1.e-9);
  g.snap_selection(1);assert(g.nodes().at("a").position.x==2);
  rejects([&]{g.snap_selection(0);});
  g.connect({"l","a","out","b","in"});
  g.rewire("l",{"l","a","out","c","in"});
  assert(g.links().at("l").to_node=="c");
  rejects([&]{g.rewire("l",{"wrong","a","out","b","in"});});
  assert(g.links().at("l").to_node=="c");
  assert(g.undo());assert(g.links().at("l").to_node=="b");
  assert(g.redo());assert(g.links().at("l").to_node=="c");
  assert(deserialize_graph(serialize_graph(g)).links().size()==1);

  PianoRoll roll;roll.insert({"n",60,0,1,100});
  roll.move_note("n",62,2);roll.resize_note("n",3);
  assert(roll.notes().at("n").pitch==62&&roll.notes_at(4).size()==1);
  rejects([&]{roll.move_note("n",128,0);});
  roll.select_note("n");assert(roll.selection().count("n"));
  roll.erase("n");assert(roll.selection().empty());
  assert(roll.undo());assert(roll.selection().count("n"));
  roll.set_meter({7,8});assert(roll.beats_per_bar()==3.5);
  roll.set_bpm(140);assert(roll.beat_to_bar(7)==2);
  assert(std::abs(roll.beat_to_seconds(14)-6)<1.e-9);
  assert(std::abs(roll.seconds_to_beat(6)-14)<1.e-9);
  rejects([&]{roll.set_meter({4,3});});
  rejects([&]{roll.set_bpm(0);});

  TileMenuModel tiles;tiles.add({"a","First",true});tiles.add({"b","Disabled",false});tiles.add({"c","Third",true});
  rejects([&]{tiles.add({"a","Duplicate",true});});
  assert(tiles.next().value()=="a");
  assert(tiles.choose("a",tiles.revision()));
  assert(tiles.next().value()=="c");
  rejects([&]{tiles.choose("b",tiles.revision());});
  const auto old=tiles.revision();tiles.set_enabled("a",false);
  assert(!tiles.choose("c",old));
  assert(tiles.next().value()=="c");

  auto reg=default_registry();Tree tree(reg);tree.create("root","Panel");tree.set_root("root");
  tree.create("menu","TileMenu");tree.append("root","menu");
  tree.create("select","Select");tree.append("root","select");
  tree.create("text","TextArea");tree.append("root","text");
  Controls controls(tree);int selections=0,edits=0,submits=0;
  tree.on("menu","select",[&](const Event& event){assert(std::get<std::string>(event.payload)=="c");++selections;});
  tree.on("select","change",[&](const Event&){++edits;});
  tree.on("text","submit",[&](const Event&){++submits;});
  assert(controls.tile_select("menu",tiles,"c",tree.revision(),tiles.revision()));
  assert(selections==1&&tiles.selected().value()=="c");
  assert(!controls.tile_select("menu",tiles,"c",tree.revision()-1,tiles.revision()));
  assert(controls.select("select","hello",tree.revision()));assert(edits==1);
  assert(controls.text_area("text","new body",tree.revision()));
  assert(controls.submit_text("text")&&submits==1);
  rejects([&]{controls.tile_select("menu",tiles,"b",tree.revision(),tiles.revision());});
  return 0;
}
