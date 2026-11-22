#pragma once
#include <floks/components.hpp>
#include <cmath>

namespace floks {
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
  bool text_input(const std::string& id,std::string value,std::uint64_t expected_revision) {
    const auto c=tree_.require(id);
    if(c->type()!="TextInput")throw std::invalid_argument("not a text input");
    if(!tree_.execute({id,"value",value,expected_revision}))return false;
    tree_.dispatch(id,"change",std::move(value));return true;
  }
};
}
