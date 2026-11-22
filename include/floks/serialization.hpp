#pragma once
#include <floks/graph.hpp>
#include <floks/music.hpp>
#include <iomanip>
#include <sstream>
#include <string>

namespace floks {
inline std::string serialize_graph(const Graph& graph) {
  std::ostringstream out;out<<std::setprecision(17);
  const auto v=graph.viewport();
  out<<"FLOKS_GRAPH1 "<<v.offset.x<<' '<<v.offset.y<<' '<<v.zoom<<' '\
     <<graph.nodes().size()<<' '<<graph.links().size()<<' '<<graph.selection().size()<<'\n';
  for(const auto& [id,n]:graph.nodes()) {
    (void)id;
    out<<"N "<<std::quoted(n.id)<<' '<<std::quoted(n.label)<<' '<<n.position.x<<' '<<n.position.y<<' '<<n.ports.size()<<'\n';
    for(const auto& p:n.ports)out<<"P "<<std::quoted(p.id)<<' '<<std::quoted(p.type)<<' '<<static_cast<int>(p.direction)<<' '<<p.capacity<<'\n';
  }
  for(const auto& [id,l]:graph.links()) {
    (void)id;out<<"L "<<std::quoted(l.id)<<' '<<std::quoted(l.from_node)<<' '<<std::quoted(l.from_port)<<' '<<std::quoted(l.to_node)<<' '<<std::quoted(l.to_port)<<'\n';
  }
  for(const auto& id:graph.selection())out<<"S "<<std::quoted(id)<<'\n';
  return out.str();
}
inline Graph deserialize_graph(const std::string& text) {
  std::istringstream in(text);std::string version;double x,y,zoom;std::size_t nn,nl,ns;
  if(!(in>>version>>x>>y>>zoom>>nn>>nl>>ns)||version!="FLOKS_GRAPH1"||nn>10000||nl>100000||ns>nn)
    throw std::invalid_argument("invalid graph header");
  Graph g;
  for(std::size_t i=0;i<nn;++i) {
    char tag;Node n;std::size_t np;
    if(!(in>>tag>>std::quoted(n.id)>>std::quoted(n.label)>>n.position.x>>n.position.y>>np)||tag!='N'||np>1024)
      throw std::invalid_argument("invalid graph node");
    for(std::size_t j=0;j<np;++j) {
      char pt;Port p;int direction;
      if(!(in>>pt>>std::quoted(p.id)>>std::quoted(p.type)>>direction>>p.capacity)||pt!='P'||(direction!=0&&direction!=1))
        throw std::invalid_argument("invalid graph port");
      p.direction=static_cast<Port::Direction>(direction);
      n.ports.push_back(std::move(p));
    }
    g.add_node(std::move(n));
  }
  for(std::size_t i=0;i<nl;++i) {
    char lt;Link l;
    if(!(in>>lt>>std::quoted(l.id)>>std::quoted(l.from_node)>>std::quoted(l.from_port)>>std::quoted(l.to_node)>>std::quoted(l.to_port))||lt!='L')
      throw std::invalid_argument("invalid graph link");
    g.connect(std::move(l));
  }
  for(std::size_t i=0;i<ns;++i) {
    char st;std::string id;
    if(!(in>>st>>std::quoted(id))||st!='S'||g.selection().count(id))throw std::invalid_argument("invalid graph selection");
    g.select(id);
  }
  g.set_viewport({{x,y},zoom});
  in>>std::ws;if(in.peek()!=std::char_traits<char>::eof())throw std::invalid_argument("trailing graph data");
  return g;
}
inline std::string serialize_roll(const PianoRoll& roll) {
  const auto loop=roll.loop();const auto meter=roll.meter();
  std::ostringstream out;out<<std::setprecision(17);
  out<<"FLOKS_ROLL2 "<<roll.playhead()<<' '<<loop.start<<' '<<loop.end<<' '<<loop.enabled<<' '
     <<roll.notes().size()<<' '<<roll.markers().size()<<' '<<meter.numerator<<' '<<meter.denominator
     <<' '<<roll.bpm()<<' '<<roll.selection().size()<<'\n';
  for(const auto& [id,n]:roll.notes()) {
    (void)id;out<<"N "<<std::quoted(n.id)<<' '<<n.pitch<<' '<<n.start<<' '<<n.length<<' '<<n.velocity<<'\n';
  }
  for(const auto& [id,m]:roll.markers()) {
    (void)id;out<<"M "<<std::quoted(m.id)<<' '<<std::quoted(m.label)<<' '<<m.beat<<'\n';
  }
  for(const auto& id:roll.selection())out<<"S "<<std::quoted(id)<<'\n';
  return out.str();
}
inline PianoRoll deserialize_roll(const std::string& text) {
  std::istringstream in(text);std::string version;double playhead,ls,le;int enabled;std::size_t nn,nm,ns=0;
  if(!(in>>version)|| (version!="FLOKS_ROLL1" && version!="FLOKS_ROLL2"))
    throw std::invalid_argument("invalid roll version");
  if(!(in>>playhead>>ls>>le>>enabled>>nn>>nm)||nn>100000||nm>10000||(enabled!=0&&enabled!=1))
    throw std::invalid_argument("invalid roll header");
  int numerator=4,denominator=4;double bpm=120;
  if(version=="FLOKS_ROLL2" && !(in>>numerator>>denominator>>bpm>>ns))
    throw std::invalid_argument("invalid roll meter/tempo header");
  if(ns>nn)throw std::invalid_argument("invalid roll selection count");
  PianoRoll r;
  r.set_meter({numerator,denominator});r.set_bpm(bpm);
  for(std::size_t i=0;i<nn;++i){char tag;MidiNote n;
    if(!(in>>tag>>std::quoted(n.id)>>n.pitch>>n.start>>n.length>>n.velocity)||tag!='N')throw std::invalid_argument("invalid MIDI note data");
    r.insert(std::move(n));
  }
  for(std::size_t i=0;i<nm;++i){char tag;Marker m;
    if(!(in>>tag>>std::quoted(m.id)>>std::quoted(m.label)>>m.beat)||tag!='M')throw std::invalid_argument("invalid MIDI marker data");
    r.add_marker(std::move(m));
  }
  for(std::size_t i=0;i<ns;++i){char tag;std::string id;
    if(!(in>>tag>>std::quoted(id))||tag!='S'||r.selection().count(id))throw std::invalid_argument("invalid note selection");
    r.select_note(id);
  }
  r.set_loop({ls,le,bool(enabled)});r.seek(playhead);
  in>>std::ws;if(in.peek()!=std::char_traits<char>::eof())throw std::invalid_argument("trailing roll data");
  return r;
}
}
