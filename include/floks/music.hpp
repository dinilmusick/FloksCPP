#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace floks {
struct MidiNote {
  std::string id;
  int pitch{60};
  double start{0};
  double length{1};
  int velocity{100};
};
struct Marker { std::string id, label; double beat{0}; };
struct Loop { double start{0}, end{4}; bool enabled{false}; };

class PianoRoll {
  struct State { std::map<std::string,MidiNote> notes; std::map<std::string,Marker> markers; Loop loop; double playhead{0}; };
  State state_;
  std::vector<State> undo_,redo_;
  double pixels_per_beat_{64},pixels_per_pitch_{12};
  std::uint64_t revision_{0};
  static void validate_note(const MidiNote& n) {
    if(n.id.empty()||n.pitch<0||n.pitch>127||n.velocity<0||n.velocity>127||!std::isfinite(n.start)||n.start<0||!std::isfinite(n.length)||n.length<=0) throw std::invalid_argument("invalid MIDI note");
  }
  void checkpoint(){undo_.push_back(state_);if(undo_.size()>100)undo_.erase(undo_.begin());redo_.clear();++revision_;}
public:
  std::uint64_t revision() const noexcept {return revision_;}
  const std::map<std::string,MidiNote>& notes() const {return state_.notes;}
  const std::map<std::string,Marker>& markers() const {return state_.markers;}
  Loop loop() const {return state_.loop;}
  double playhead() const {return state_.playhead;}
  void insert(MidiNote n) {
    validate_note(n); if(state_.notes.count(n.id)) throw std::invalid_argument("duplicate MIDI note");
    checkpoint();state_.notes.emplace(n.id,std::move(n));
  }
  void update(const std::string& id,int pitch,double start,double length,int velocity) {
    if(!state_.notes.count(id)) throw std::out_of_range("unknown MIDI note");
    MidiNote n{id,pitch,start,length,velocity};validate_note(n);checkpoint();state_.notes[id]=n;
  }
  void erase(const std::string& id) {
    if(!state_.notes.count(id)) throw std::out_of_range("unknown MIDI note");
    checkpoint();state_.notes.erase(id);
  }
  void add_marker(Marker m) {
    if(m.id.empty()||!std::isfinite(m.beat)||m.beat<0||state_.markers.count(m.id)) throw std::invalid_argument("invalid marker");
    checkpoint();state_.markers.emplace(m.id,std::move(m));
  }
  void move_marker(const std::string& id,double beat) {
    if(!state_.markers.count(id)) throw std::out_of_range("marker missing");
    if(!std::isfinite(beat)||beat<0) throw std::invalid_argument("invalid marker beat");
    checkpoint();state_.markers.at(id).beat=beat;
  }
  void rename_marker(const std::string& id,std::string label) {
    if(!state_.markers.count(id)) throw std::out_of_range("marker missing");
    checkpoint();state_.markers.at(id).label=std::move(label);
  }
  void remove_marker(const std::string& id) {
    if(!state_.markers.count(id)) throw std::out_of_range("marker missing");
    checkpoint();state_.markers.erase(id);
  }
  void set_loop(Loop v) {
    if(!std::isfinite(v.start)||!std::isfinite(v.end)||v.start<0||v.end<=v.start) throw std::invalid_argument("invalid loop");
    checkpoint();state_.loop=v;
    if(v.enabled && (state_.playhead<v.start||state_.playhead>=v.end))state_.playhead=v.start;
  }
  void seek(double beat) {
    if(!std::isfinite(beat)||beat<0) throw std::invalid_argument("invalid seek");
    checkpoint();state_.playhead=beat;
  }
  void advance(double beats) {
    if(!std::isfinite(beats)||beats<0) throw std::invalid_argument("invalid advance");
    double next=state_.playhead+beats;
    if(!std::isfinite(next)) throw std::invalid_argument("playhead overflow");
    if(state_.loop.enabled && next>=state_.loop.end) {
      next=state_.loop.start+std::fmod(next-state_.loop.start,state_.loop.end-state_.loop.start);
    }
    checkpoint(); state_.playhead=next;
  }
  void set_scale(double pxbeat,double pxpitch) {
    if(!std::isfinite(pxbeat)||!std::isfinite(pxpitch)||pxbeat<=0||pxpitch<=0) throw std::invalid_argument("invalid scale");
    pixels_per_beat_=pxbeat;pixels_per_pitch_=pxpitch;
  }
  double beat_to_x(double beat) const { if(!std::isfinite(beat)||beat<0||!std::isfinite(beat*pixels_per_beat_))throw std::invalid_argument("invalid beat coordinate");return beat*pixels_per_beat_; }
  double x_to_beat(double x) const { if(!std::isfinite(x)||x<0)throw std::invalid_argument("invalid x coordinate");return x/pixels_per_beat_; }
  double pitch_to_y(int pitch) const { if(pitch<0||pitch>127)throw std::out_of_range("MIDI pitch");return (127-pitch)*pixels_per_pitch_; }
  int y_to_pitch(double y) const { if(!std::isfinite(y))throw std::invalid_argument("invalid y");const auto pitch=127.0-std::floor(y/pixels_per_pitch_);if(pitch<=0)return 0;if(pitch>=127)return 127;return static_cast<int>(pitch); }
  double snap(double beat,double grid) const {
    if(!std::isfinite(beat)||!std::isfinite(grid)||beat<0||grid<=0) throw std::invalid_argument("invalid quantization");
    const double result=std::round(beat/grid)*grid;
    if(!std::isfinite(result))throw std::invalid_argument("quantization overflow");
    return result;
  }
  std::vector<MidiNote> notes_at(double beat) const {
    if(!std::isfinite(beat)) throw std::invalid_argument("invalid beat");
    std::vector<MidiNote> found;
    for(const auto& [id,n]:state_.notes){(void)id;if(n.start<=beat&&beat<n.start+n.length)found.push_back(n);}
    return found;
  }
  bool undo(){if(undo_.empty())return false;redo_.push_back(state_);state_=std::move(undo_.back());undo_.pop_back();++revision_;return true;}
  bool redo(){if(redo_.empty())return false;undo_.push_back(state_);state_=std::move(redo_.back());redo_.pop_back();++revision_;return true;}
};
}
