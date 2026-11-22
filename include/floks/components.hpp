#pragma once
#include <algorithm>
#include <cstdint>
#include <cmath>
#include <functional>
#include <iomanip>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace floks {
using Value = std::variant<bool, std::int64_t, double, std::string>;
using Props = std::map<std::string, Value>;

inline std::string json_escape(const std::string& s) {
  std::string out = "\"";
  constexpr char hex[] = "0123456789abcdef";
  for (unsigned char c : s) {
    if (c == '"' || c == '\\') { out += '\\'; out += static_cast<char>(c); }
    else if (c < 32) { out += "\\u00"; out += hex[c >> 4]; out += hex[c & 15]; }
    else out += static_cast<char>(c);
  }
  return out + '"';
}
inline std::string json_value(const Value& v) {
  return std::visit([](const auto& val) -> std::string {
    using T = std::decay_t<decltype(val)>;
    if constexpr (std::is_same_v<T, std::string>) return json_escape(val);
    else if constexpr (std::is_same_v<T, bool>) return val ? "true" : "false";
    else { if constexpr (std::is_same_v<T,double>) { if(!std::isfinite(val)) throw std::invalid_argument("non-finite JSON number"); } std::ostringstream out; out << std::setprecision(17) << val; return out.str(); }
  }, v);
}

struct ComponentDefinition {
  std::string type;
  Props defaults;
  std::set<std::string> events;
  std::string description;
};

class Registry {
  std::map<std::string, ComponentDefinition> defs_;
public:
  void register_component(ComponentDefinition d) {
    if (d.type.empty() || defs_.count(d.type)) throw std::invalid_argument("component type empty or already registered");
    defs_.emplace(d.type, std::move(d));
  }
  const ComponentDefinition& require(const std::string& type) const {
    auto it = defs_.find(type);
    if (it == defs_.end()) throw std::out_of_range("unknown component type: " + type);
    return it->second;
  }
  std::size_t size() const { return defs_.size(); }
};

struct Rectangle { double x{0}, y{0}, width{0}, height{0}; };
enum class Flow { Row, Column, Overlay, Absolute };
struct Layout { Flow flow{Flow::Column}; double spacing{0}; Rectangle bounds{}; };
struct Theme {
  std::map<std::string, Value> tokens;
  Value resolve(const std::string& key, const Theme* parent = nullptr) const {
    auto it = tokens.find(key);
    if (it != tokens.end()) return it->second;
    if (parent) return parent->resolve(key);
    throw std::out_of_range("theme token missing: " + key);
  }
};

struct Event { std::string name; Value payload; std::uint64_t revision{0}; };
class Component : public std::enable_shared_from_this<Component> {
  friend class Tree;
  std::string id_, type_;
  Props props_;
  Layout layout_;
  Theme theme_;
  std::weak_ptr<Component> parent_;
  std::vector<std::shared_ptr<Component>> children_;
  bool mounted_{false};
  Component(std::string id, std::string type, Props props)
    : id_(std::move(id)), type_(std::move(type)), props_(std::move(props)) {}
public:
  const std::string& id() const noexcept { return id_; }
  const std::string& type() const noexcept { return type_; }
  bool mounted() const noexcept { return mounted_; }
  const Props& props() const noexcept { return props_; }
  const Layout& layout() const noexcept { return layout_; }
  const Theme& theme() const noexcept { return theme_; }
  std::shared_ptr<const Component> parent() const { return parent_.lock(); }
  std::size_t child_count() const noexcept { return children_.size(); }
};

struct Snapshot {
  std::string id, type;
  Props props;
  Layout layout;
  Theme theme;
  std::vector<Snapshot> children;
};

struct Command { std::string target, property; Value value; std::uint64_t expected_revision{0}; };
class Tree {
public:
  using Handler = std::function<void(const Event&)>;
  struct HandlerStore {
    std::map<std::string,std::map<std::string,std::map<std::uint64_t,Handler>>> events;
  };
  class ScopedListener {
    std::weak_ptr<HandlerStore> store_;
    std::string id_, event_;
    std::uint64_t token_{0};
  public:
    ScopedListener()=default;
    ScopedListener(std::weak_ptr<HandlerStore> store,std::string id,std::string event,std::uint64_t token)
      :store_(std::move(store)),id_(std::move(id)),event_(std::move(event)),token_(token){}
    ScopedListener(const ScopedListener&)=delete;
    ScopedListener& operator=(const ScopedListener&)=delete;
    ScopedListener(ScopedListener&& other) noexcept
      :store_(std::move(other.store_)),id_(std::move(other.id_)),event_(std::move(other.event_)),token_(other.token_){other.token_=0;}
    ScopedListener& operator=(ScopedListener&& other) noexcept {
      if(this!=&other){reset();store_=std::move(other.store_);id_=std::move(other.id_);event_=std::move(other.event_);token_=other.token_;other.token_=0;}
      return *this;
    }
    ~ScopedListener(){reset();}
    void reset() noexcept {
      if(token_){if(auto state=store_.lock())state->events[id_][event_].erase(token_);token_=0;}
    }
  };
private:
  const Registry* registry_;
  std::map<std::string, std::shared_ptr<Component>> index_;
  std::shared_ptr<Component> root_;
  std::shared_ptr<HandlerStore> handlers_{std::make_shared<HandlerStore>()};
  std::uint64_t next_handler_{1}, revision_{0};
  bool dispatching_{false};
  static void fill_snapshot(const std::shared_ptr<Component>& n, Snapshot& s) {
    s.id=n->id_; s.type=n->type_; s.props=n->props_; s.layout=n->layout_; s.theme=n->theme_;
    for (const auto& c : n->children_) { s.children.emplace_back(); fill_snapshot(c,s.children.back()); }
  }
  static void line_write(const Snapshot& s, std::ostream& out) {
    out << "C " << std::quoted(s.id) << ' ' << std::quoted(s.type) << ' ' << s.props.size() << ' ' << s.children.size() << '\n';
    for (const auto& [k,v]:s.props) {
      out << "P " << std::quoted(k) << ' ' << v.index() << ' ';
      std::visit([&out](const auto& data) { if constexpr (std::is_same_v<std::decay_t<decltype(data)>,std::string>) out << std::quoted(data); else out << data; },v);
      out << '\n';
    }
    out << "L " << static_cast<int>(s.layout.flow) << ' ' << s.layout.spacing << ' '
        << s.layout.bounds.x << ' ' << s.layout.bounds.y << ' '
        << s.layout.bounds.width << ' ' << s.layout.bounds.height << '\n';
    out << "T " << s.theme.tokens.size() << '\n';
    for(const auto& [key,value]:s.theme.tokens) {
      out << "P " << std::quoted(key) << ' ' << value.index() << ' ';
      std::visit([&out](const auto& data) { if constexpr(std::is_same_v<std::decay_t<decltype(data)>,std::string>) out << std::quoted(data); else out << data; },value);
      out << '\n';
    }
    for (const auto& c:s.children) line_write(c,out);
  }
  std::shared_ptr<Component> line_read(std::istream& in, const std::optional<std::string>& parent, std::size_t depth=0) {
    if(depth>128||index_.size()>10000) throw std::invalid_argument("component nesting or size exceeded");
    char tag; std::string id,type; std::size_t np,nc;
    if (!(in >> tag >> std::quoted(id) >> std::quoted(type) >> np >> nc) || tag!='C' || np>1024 || nc>100000)
      throw std::invalid_argument("malformed component stream");
    if (index_.count(id)) throw std::invalid_argument("duplicate serialized component id");
    Props p;
    for(std::size_t i=0;i<np;++i){
      char pt; std::string k; int kind;
      if(!(in>>pt>>std::quoted(k)>>kind)||pt!='P'||p.count(k)) throw std::invalid_argument("malformed serialized property");
      switch(kind) {
        case 0: { int x; if(!(in>>x)|| (x!=0 && x!=1)) throw std::invalid_argument("invalid bool"); p[k]=bool(x); break; }
        case 1: { std::int64_t x; if(!(in>>x)) throw std::invalid_argument("invalid integer"); p[k]=x; break; }
        case 2: { double x; if(!(in>>x)) throw std::invalid_argument("invalid double"); p[k]=x; break; }
        case 3: { std::string x; if(!(in>>std::quoted(x))) throw std::invalid_argument("invalid string"); p[k]=x; break; }
        default: throw std::invalid_argument("invalid property type");
      }
    }
    auto node=create(id,type,std::move(p));
    if(parent) append(*parent,id); else set_root(id);
    char layout_tag;int flow;Layout layout;
    if(!(in>>layout_tag>>flow>>layout.spacing>>layout.bounds.x>>layout.bounds.y>>layout.bounds.width>>layout.bounds.height)
       ||layout_tag!='L'||flow<0||flow>3)throw std::invalid_argument("invalid layout data");
    layout.flow=static_cast<Flow>(flow);
    set_layout(id,layout);
    char theme_tag;std::size_t nt;
    if(!(in>>theme_tag>>nt)||theme_tag!='T'||nt>1024)throw std::invalid_argument("invalid theme count");
    for(std::size_t i=0;i<nt;++i) {
      char pt;std::string key;int kind;
      if(!(in>>pt>>std::quoted(key)>>kind)||pt!='P'||node->theme_.tokens.count(key))throw std::invalid_argument("invalid theme token");
      switch(kind) {
        case 0: {int v;if(!(in>>v)||(v!=0&&v!=1))throw std::invalid_argument("invalid theme bool");set_theme_token(id,key,bool(v));break;}
        case 1: {std::int64_t v;if(!(in>>v))throw std::invalid_argument("invalid theme integer");set_theme_token(id,key,v);break;}
        case 2: {double v;if(!(in>>v))throw std::invalid_argument("invalid theme double");set_theme_token(id,key,v);break;}
        case 3: {std::string v;if(!(in>>std::quoted(v)))throw std::invalid_argument("invalid theme text");set_theme_token(id,key,v);break;}
        default: throw std::invalid_argument("invalid theme value type");
      }
    }
    for(std::size_t i=0;i<nc;++i) line_read(in,id,depth+1);
    return node;
  }
public:
  explicit Tree(const Registry& registry):registry_(&registry) {}
  std::uint64_t revision() const noexcept { return revision_; }
  std::shared_ptr<Component> create(std::string id, const std::string& type, Props props={}) {
    if(id.empty() || index_.count(id)) throw std::invalid_argument("duplicate or empty component id");
    if(dispatching_) throw std::logic_error("structural mutation during event dispatch");
    const auto& d = registry_->require(type);
    for(const auto& [key,v]:props) {
      auto it=d.defaults.find(key);
      if(it==d.defaults.end() || it->second.index()!=v.index()) throw std::invalid_argument("unknown or mismatched property: " + key);
    }
    auto merged=d.defaults;
    for(const auto& [k,v]:props) {if(const auto* real=std::get_if<double>(&v); real && !std::isfinite(*real)) throw std::invalid_argument("non-finite property"); merged[k]=v;}
    auto n=std::shared_ptr<Component>(new Component(std::move(id),type,std::move(merged)));
    index_.emplace(n->id_,n);
    ++revision_;
    return n;
  }
  void set_root(const std::string& id) {
    auto n=require(id);
    if(dispatching_) throw std::logic_error("structural mutation during event dispatch");
    if(root_ || n->parent_.lock() || n->mounted_) throw std::logic_error("invalid root ownership");
    root_=n; mount(n); ++revision_;
  }
  std::shared_ptr<Component> require(const std::string& id) const {
    auto it=index_.find(id); if(it==index_.end()) throw std::out_of_range("component missing: " + id); return it->second;
  }
  void append(const std::string& parent, const std::string& child) {
    if(dispatching_) throw std::logic_error("structural mutation during event dispatch");
    auto p=require(parent), c=require(child);
    if(p==c || c->parent_.lock() || c==root_) throw std::logic_error("component already owned or recursive");
    if(!p->mounted_ && !root_) throw std::logic_error("root must be set before attaching children");
    p->children_.push_back(c); c->parent_=p; if(p->mounted_) mount(c); ++revision_;
  }
  void remove(const std::string& id) {
    if(dispatching_) throw std::logic_error("structural mutation during event dispatch");
    auto n=require(id);
    if(n==root_) root_.reset();
    if(auto p=n->parent_.lock()) {
      auto& kids=p->children_;
      kids.erase(std::remove(kids.begin(),kids.end(),n),kids.end());
    }
    n->parent_.reset();
    detach(n); ++revision_;
  }
  void set_property(const std::string& id,const std::string& key, Value value) {
    auto n=require(id); auto it=registry_->require(n->type_).defaults.find(key);
    if(it==registry_->require(n->type_).defaults.end() || it->second.index()!=value.index()) throw std::invalid_argument("invalid typed property");
    if (const auto* d=std::get_if<double>(&value);d && !std::isfinite(*d)) throw std::invalid_argument("non-finite property");
    n->props_[key]=std::move(value); ++revision_;
  }
  bool execute(const Command& c) {
    if(c.expected_revision!=revision_) return false;
    set_property(c.target,c.property,c.value);
    return true;
  }
  void set_layout(const std::string& id,Layout layout) {
    if(!std::isfinite(layout.spacing)||!std::isfinite(layout.bounds.x)||!std::isfinite(layout.bounds.y)||!std::isfinite(layout.bounds.width)||!std::isfinite(layout.bounds.height)||layout.spacing<0||layout.bounds.width<0||layout.bounds.height<0) throw std::invalid_argument("negative geometry");
    require(id)->layout_=layout; ++revision_;
  }
  void set_theme_token(const std::string& id,std::string key,Value v) {
    if(key.empty()) throw std::invalid_argument("empty theme key");
    if (const auto* d=std::get_if<double>(&v);d && !std::isfinite(*d)) throw std::invalid_argument("non-finite theme value");
    require(id)->theme_.tokens[std::move(key)]=std::move(v); ++revision_;
  }
  Value theme_value(const std::string& id,const std::string& key) const {
    for(auto p=require(id);p;p=p->parent_.lock()) {
      auto it=p->theme_.tokens.find(key); if(it!=p->theme_.tokens.end()) return it->second;
    }
    throw std::out_of_range("theme token missing");
  }
  std::uint64_t on(const std::string& id,const std::string& event,Handler h) {
    auto n=require(id);
    if(!h || !registry_->require(n->type_).events.count(event)) throw std::invalid_argument("unknown event or empty handler");
    auto token=next_handler_++; handlers_->events[id][event].emplace(token,std::move(h)); return token;
  }
  ScopedListener on_scoped(const std::string& id,const std::string& event,Handler h) {
    auto token=on(id,event,std::move(h));return ScopedListener{handlers_,id,event,token};
  }
  void off(const std::string& id,const std::string& event,std::uint64_t token) { handlers_->events[id][event].erase(token); }
  void dispatch(const std::string& id,const std::string& name,Value value) {
    auto n=require(id);
    if(!n->mounted_ || !registry_->require(n->type_).events.count(name)) throw std::logic_error("event on unmounted or non-event component");
    if(dispatching_) throw std::logic_error("reentrant dispatch rejected");
    struct Reset { bool& flag; ~Reset() {flag=false;} } reset{dispatching_};
    dispatching_=true;
    Event evt{name,std::move(value),revision_};
    auto copied=handlers_->events[id][name];
    for(const auto& [token,handler]:copied) { (void)token; if(!n->mounted_) break; handler(evt); }
  }
  Snapshot snapshot() const {
    if(!root_) throw std::logic_error("root missing");
    Snapshot snap; fill_snapshot(root_,snap); return snap;
  }
  std::string serialize() const { std::ostringstream o; o << std::setprecision(17) << "FLOKSCPP2\n"; line_write(snapshot(),o); return o.str(); }
  static Tree deserialize(const Registry& registry,const std::string& wire) {
    Tree t(registry); std::istringstream in(wire); std::string version;
    if(!std::getline(in,version)||version!="FLOKSCPP2") throw std::invalid_argument("bad wire version");
    auto root=t.line_read(in,std::nullopt);
    (void)root;
    in>>std::ws; if(in.peek()!=std::char_traits<char>::eof()) throw std::invalid_argument("trailing component data");
    return t;
  }
  std::size_t size() const { return index_.size(); }
private:
  void mount(const std::shared_ptr<Component>& p) { p->mounted_=true; for(auto& c:p->children_) mount(c); }
  void detach(const std::shared_ptr<Component>& p) {
    for(auto& c:p->children_) detach(c);
    p->children_.clear(); p->mounted_=false; p->parent_.reset(); handlers_->events.erase(p->id_); index_.erase(p->id_);
  }
};

class Renderer { public: virtual ~Renderer()=default; virtual void present(const Snapshot&,std::uint64_t revision)=0; };
inline void present(const Tree& t,Renderer& r) { r.present(t.snapshot(),t.revision()); }

inline Registry default_registry() {
  Registry r;
  r.register_component({"Panel",{{"visible",true}}, {},"Layout container"});
  r.register_component({"Card",{{"title",std::string{}},{"visible",true}}, {},"Card container"});
  r.register_component({"Badge",{{"label",std::string{}}}, {},"Badge label"});
  r.register_component({"Text",{{"text",std::string{}}},{},"Accessible text"});
  r.register_component({"Button",{{"label",std::string{}},{"enabled",true}}, {"activate"},"Activatable control"});
  r.register_component({"Toggle",{{"checked",false}}, {"change"},"Boolean toggle"});
  r.register_component({"Slider",{{"value",0.0},{"minimum",0.0},{"maximum",1.0}}, {"change"},"Scalar input"});
  r.register_component({"TextInput",{{"value",std::string{}},{"placeholder",std::string{}}}, {"change","submit"},"Text input"});
  r.register_component({"TextArea",{{"value",std::string{}},{"placeholder",std::string{}}}, {"change","submit"},"Multiline text editor"});
  r.register_component({"Select",{{"value",std::string{}}}, {"change"},"Selection control"});
  r.register_component({"TileMenu",{{"selected",std::string{}}}, {"select"},"Tile selection"});
  r.register_component({"NodeGraph",{{"editable",true}}, {"select","connect","disconnect"},"Graph host control"});
  r.register_component({"PianoRoll",{{"editable",true}}, {"note","seek","loop"},"MIDI piano roll host control"});
  r.register_component({"Timeline",{{"editable",true}}, {"marker","seek"},"Musical timeline host control"});
  return r;
}
}
