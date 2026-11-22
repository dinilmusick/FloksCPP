#pragma once
// Optional concrete adapter. The header depends on the verified installed FrnlCPP::semantic package.
#include <frnl/semantic_runtime.hpp>
#include <any>
#include <functional>
#include <stdexcept>
#include <string>
#include <utility>

namespace floks {
class FrnlBridge {
  frnl::SemanticRuntime& runtime_;
public:
  explicit FrnlBridge(frnl::SemanticRuntime& runtime):runtime_(runtime) {}
  void register_button(const std::string& id,std::function<void()> action) {
    if(id.empty()||!action) throw std::invalid_argument("invalid Frnl bridge target");
    frnl::ComponentDefinition definition;
    definition.name="Button";
    const std::string handler="floks_"+id+"_activate";
    definition.states["idle"].events["activate"]=handler;
    runtime_.register_component(id,definition,"idle");
    runtime_.register_handler(handler,[action=std::move(action)](frnl::SemanticRuntime&,const std::string&,const std::any&)->std::any {
      action(); return {};
    });
  }
  void activate(const std::string& id) { (void)runtime_.invoke(id,"activate"); }
};
}
