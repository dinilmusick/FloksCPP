#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstddef>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace floks {
struct Point { double x{0}, y{0}; };
struct Port {
  std::string id, type;
  enum class Direction { Input, Output } direction{Direction::Input};
  std::size_t capacity{0}; // zero means unlimited
};
struct Node {
  std::string id, label;
  Point position;
  std::vector<Port> ports;
};
struct Link { std::string id, from_node, from_port, to_node, to_port; };
struct Viewport { Point offset{}; double zoom{1}; };

class Graph {
  struct State {
    std::map<std::string,Node> nodes;
    std::map<std::string,Link> links;
    std::set<std::string> selected;
    Viewport viewport;
  };
  State current_;
  std::vector<State> undo_, redo_;
  std::size_t history_limit_{100};
  std::uint64_t revision_{0};
  void checkpoint() {
    undo_.push_back(current_);
    if(undo_.size()>history_limit_) undo_.erase(undo_.begin());
    redo_.clear(); ++revision_;
  }
  const Port& port(const std::string& node,const std::string& port_id) const {
    auto it=current_.nodes.find(node);
    if(it==current_.nodes.end()) throw std::out_of_range("unknown node");
    auto p=std::find_if(it->second.ports.begin(),it->second.ports.end(),[&](const Port& q){return q.id==port_id;});
    if(p==it->second.ports.end()) throw std::out_of_range("unknown port");
    return *p;
  }
  std::size_t used(const std::string& node,const std::string& port_id) const {
    std::size_t n=0;
    for(const auto& [key,l]:current_.links) { (void)key;
      n+= (l.from_node==node&&l.from_port==port_id) || (l.to_node==node&&l.to_port==port_id);
    }
    return n;
  }
  bool reaches(const std::string& from,const std::string& target,std::set<std::string>& seen) const {
    if(from==target) return true;
    if(!seen.insert(from).second) return false;
    for(const auto& [key,l]:current_.links) { (void)key;
      if(l.from_node==from && reaches(l.to_node,target,seen)) return true;
    }
    return false;
  }
public:
  std::uint64_t revision() const noexcept {return revision_;}
  const std::map<std::string,Node>& nodes() const {return current_.nodes;}
  const std::map<std::string,Link>& links() const {return current_.links;}
  const std::set<std::string>& selection() const {return current_.selected;}
  Viewport viewport() const {return current_.viewport;}
  void add_node(Node node) {
    if(node.id.empty()||current_.nodes.count(node.id)||!std::isfinite(node.position.x)||!std::isfinite(node.position.y)) throw std::invalid_argument("invalid node");
    std::set<std::string> ports;
    for(const auto& p:node.ports) if(p.id.empty()||p.type.empty()||!ports.insert(p.id).second) throw std::invalid_argument("invalid or duplicate port");
    checkpoint(); current_.nodes.emplace(node.id,std::move(node));
  }
  void move_node(const std::string& id,Point p) {
    if(!std::isfinite(p.x)||!std::isfinite(p.y)) throw std::invalid_argument("non-finite position");
    if(!current_.nodes.count(id)) throw std::out_of_range("missing node");
    checkpoint(); current_.nodes.at(id).position=p;
  }
  void remove_node(const std::string& id) {
    if(!current_.nodes.count(id)) throw std::out_of_range("missing node");
    checkpoint(); current_.nodes.erase(id); current_.selected.erase(id);
    for(auto it=current_.links.begin();it!=current_.links.end();) {
      if(it->second.from_node==id||it->second.to_node==id) it=current_.links.erase(it);
      else ++it;
    }
  }
  bool can_connect(const std::string& a,const std::string& ap,const std::string& b,const std::string& bp) const {
    if(a==b||!current_.nodes.count(a)||!current_.nodes.count(b)) return false;
    try {
      const auto& out=port(a,ap); const auto& in=port(b,bp);
      if(out.direction!=Port::Direction::Output||in.direction!=Port::Direction::Input||out.type!=in.type) return false;
      if((out.capacity&&used(a,ap)>=out.capacity)||(in.capacity&&used(b,bp)>=in.capacity)) return false;
      for(const auto& [id,l]:current_.links) { (void)id; if(l.from_node==a&&l.from_port==ap&&l.to_node==b&&l.to_port==bp) return false; }
      std::set<std::string> seen; if(reaches(b,a,seen)) return false;
      return true;
    } catch(const std::out_of_range&) { return false; }
  }
  void connect(Link l) {
    if(l.id.empty()||current_.links.count(l.id)||!can_connect(l.from_node,l.from_port,l.to_node,l.to_port)) throw std::invalid_argument("illegal connection");
    checkpoint(); current_.links.emplace(l.id,std::move(l));
  }
  void disconnect(const std::string& id) {
    if(!current_.links.count(id)) throw std::out_of_range("missing connection");
    checkpoint(); current_.links.erase(id);
  }
  void select(const std::string& node,bool selected=true) {
    if(!current_.nodes.count(node)) throw std::out_of_range("missing node");
    checkpoint(); if(selected) current_.selected.insert(node); else current_.selected.erase(node);
  }
  void clear_selection() { if(current_.selected.empty()) return; checkpoint(); current_.selected.clear(); }
  void set_viewport(Viewport v) {
    if(!std::isfinite(v.zoom)||v.zoom<0.01||v.zoom>100||!std::isfinite(v.offset.x)||!std::isfinite(v.offset.y)) throw std::invalid_argument("invalid viewport");
    checkpoint(); current_.viewport=v;
  }
  Point world_to_screen(Point p) const {
    if(!std::isfinite(p.x)||!std::isfinite(p.y))throw std::invalid_argument("invalid world point");
    Point out{p.x*current_.viewport.zoom+current_.viewport.offset.x,p.y*current_.viewport.zoom+current_.viewport.offset.y};
    if(!std::isfinite(out.x)||!std::isfinite(out.y))throw std::invalid_argument("screen coordinate overflow");
    return out;
  }
  Point screen_to_world(Point p) const {
    if(!std::isfinite(p.x)||!std::isfinite(p.y))throw std::invalid_argument("invalid screen point");
    Point out{(p.x-current_.viewport.offset.x)/current_.viewport.zoom,(p.y-current_.viewport.offset.y)/current_.viewport.zoom};
    if(!std::isfinite(out.x)||!std::isfinite(out.y))throw std::invalid_argument("world coordinate overflow");
    return out;
  }
  bool undo() {
    if(undo_.empty()) return false;
    redo_.push_back(current_); current_=std::move(undo_.back()); undo_.pop_back(); ++revision_; return true;
  }
  bool redo() {
    if(redo_.empty()) return false;
    undo_.push_back(current_); current_=std::move(redo_.back()); redo_.pop_back(); ++revision_; return true;
  }
  void set_history_limit(std::size_t n) {
    if(n==0) throw std::invalid_argument("history limit must be positive");
    history_limit_=n; if(undo_.size()>n) undo_.erase(undo_.begin(),undo_.end()-static_cast<std::ptrdiff_t>(n));
  }
};
}
