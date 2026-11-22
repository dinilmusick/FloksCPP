#pragma once
#include <floks/components.hpp>
#include <cmath>
#include <optional>
#include <set>
#include <vector>

namespace floks {
struct TileOption {std::string id,label; bool enabled{true};};
class TileMenuModel {
  std::vector<TileOption> options_;
  std::optional<std::string> selected_;
  std::uint64_t revision_{0};
  std::size_t position(const std::string& id) const {
    for(std::size_t i=0;i<options_.size();++i)if(options_[i].id==id)return i;
    throw std::out_of_range("missing tile");
  }
public:
  const std::vector<TileOption>& options() const noexcept {return options_;}
  const std::optional<std::string>& selected() const noexcept {return selected_;}
  std::uint64_t revision() const noexcept {return revision_;}
  void add(TileOption o) {
    if(o.id.empty()||o.label.empty())throw std::invalid_argument("invalid tile");
    for(const auto& existing:options_)if(existing.id==o.id)throw std::invalid_argument("duplicate tile");
    options_.push_back(std::move(o));++revision_;
  }
  void remove(const std::string& id) {
    const auto index=position(id);
    options_.erase(options_.begin()+static_cast<std::ptrdiff_t>(index));
    if(selected_&&*selected_==id)selected_.reset();
    ++revision_;
  }
  void set_enabled(const std::string& id,bool enabled) {
    auto& option=options_.at(position(id));
    if(option.enabled==enabled)return;
    option.enabled=enabled;
    if(!enabled&&selected_&&*selected_==id)selected_.reset();
    ++revision_;
  }
  bool can_select(const std::string& id) const {
    try{return options_.at(position(id)).enabled;}catch(const std::out_of_range&){return false;}
  }
  bool choose(const std::string& id,std::uint64_t expected_revision) {
    if(expected_revision!=revision_)return false;
    if(!can_select(id))throw std::invalid_argument("tile missing or disabled");
    if(selected_&&*selected_==id)return true;
    selected_=id;++revision_;return true;
  }
  std::optional<std::string> next(bool forward=true) const {
    if(options_.empty())return std::nullopt;
    std::size_t index=selected_?position(*selected_):(forward? options_.size()-1:0);
    for(std::size_t step=0;step<options_.size();++step) {
      index=forward?(index+1)%options_.size():(index+options_.size()-1)%options_.size();
      if(options_[index].enabled)return options_[index].id;
    }
    return std::nullopt;
  }
};
// Host-agnostic input semantics. All changes flow through the Tree revision fence.
class Controls {
  Tree& tree_;
public:
  explicit Controls(Tree& tree):tree_(tree) {}
  bool activate_button(const std::string& id) {
    const auto c=tree_.require(id);
    if(c->type()!="Button") throw std::invalid_argument("not a button");
    if(!std::get<bool>(c->props().at("enabled"))) return false;
    tree_.dispatch(id,"activate",true); return true;
  }
  bool toggle(const std::string& id,std::uint64_t expected_revision) {
    const auto c=tree_.require(id);
    if(c->type()!="Toggle") throw std::invalid_argument("not a toggle");
    const bool next=!std::get<bool>(c->props().at("checked"));
    if(!tree_.execute({id,"checked",next,expected_revision})) return false;
    tree_.dispatch(id,"change",next);return true;
  }
  bool slider(const std::string& id,double value,std::uint64_t expected_revision) {
    const auto c=tree_.require(id);
    if(c->type()!="Slider") throw std::invalid_argument("not a slider");
    const auto min=std::get<double>(c->props().at("minimum"));
    const auto max=std::get<double>(c->props().at("maximum"));
    if(!std::isfinite(value)||!std::isfinite(min)||!std::isfinite(max)||min>max||value<min||value>max)
      throw std::invalid_argument("slider out of range");
    if(!tree_.execute({id,"value",value,expected_revision}))return false;
    tree_.dispatch(id,"change",value);return true;
  }
  bool select(const std::string& id,std::string value,std::uint64_t expected_revision) {
    const auto c=tree_.require(id);
    if(c->type()!="Select")throw std::invalid_argument("not a select");
    if(!tree_.execute({id,"value",value,expected_revision}))return false;
    tree_.dispatch(id,"change",std::move(value));return true;
  }
  bool text_area(const std::string& id,std::string value,std::uint64_t expected_revision) {
    const auto c=tree_.require(id);
    if(c->type()!="TextArea")throw std::invalid_argument("not a text area");
    if(!tree_.execute({id,"value",value,expected_revision}))return false;
    tree_.dispatch(id,"change",std::move(value));return true;
  }
  bool submit_text(const std::string& id) {
    const auto c=tree_.require(id);
    if(c->type()!="TextInput"&&c->type()!="TextArea")throw std::invalid_argument("not an editable text control");
    tree_.dispatch(id,"submit",c->props().at("value"));return true;
  }
  bool tile_select(const std::string& id,TileMenuModel& menu,const std::string& tile,
                   std::uint64_t expected_tree_revision,std::uint64_t expected_menu_revision) {
    const auto c=tree_.require(id);
    if(c->type()!="TileMenu")throw std::invalid_argument("not a tile menu");
    if(expected_tree_revision!=tree_.revision()||expected_menu_revision!=menu.revision())return false;
    if(!menu.can_select(tile))throw std::invalid_argument("illegal tile selection");
    if(!tree_.execute({id,"selected",tile,expected_tree_revision}))return false;
    (void)menu.choose(tile,expected_menu_revision);
    tree_.dispatch(id,"select",tile);return true;
  }
  bool text_input(const std::string& id,std::string value,std::uint64_t expected_revision) {
    const auto c=tree_.require(id);
    if(c->type()!="TextInput")throw std::invalid_argument("not a text input");
    if(!tree_.execute({id,"value",value,expected_revision}))return false;
    tree_.dispatch(id,"change",std::move(value));return true;
  }
};
}
