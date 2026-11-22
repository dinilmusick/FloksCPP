#include <floks/frnl_bridge.hpp>
#include <cassert>
int main() {
  frnl::SemanticRuntime runtime;
  floks::FrnlBridge bridge(runtime);
  int activations=0;
  bridge.register_button("button",[&]{++activations;});
  bridge.activate("button");
  assert(activations==1);
}
