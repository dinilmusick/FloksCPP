#include <floks/music.hpp>
#include <cassert>
#include <cmath>
#include <stdexcept>
using namespace floks;
template<class F>void rejects(F f){bool ok=false;try{f();}catch(const std::exception&){ok=true;}assert(ok);}
int main(){
  PianoRoll p;
  p.insert({"note1",60,0,2,101});
  p.insert({"note2",67,1,1,110});
  assert(p.notes_at(1.5).size()==2&&p.notes_at(2).empty());
  rejects([&]{p.insert({"bad",128,0,1,90});});
  rejects([&]{p.insert({"bad",60,0,0,90});});
  rejects([&]{p.insert({"note1",65,0,1,90});});
  rejects([&]{p.update("note1",-1,0,1,90);});
  p.update("note1",61,0,3,100);assert(p.notes_at(2.5).size()==1);
  p.add_marker({"m1","Verse",4});
  rejects([&]{p.add_marker({"m1","Repeat",1});});
  p.set_loop({2,6,true});assert(p.playhead()==2);
  p.advance(5);assert(std::abs(p.playhead()-3)<0.00001);
  p.seek(5);p.advance(3);assert(std::abs(p.playhead()-4)<0.00001);
  rejects([&]{p.set_loop({2,2,true});});
  rejects([&]{p.advance(-3);});
  assert(p.pitch_to_y(127)==0&&p.y_to_pitch(0)==127);
  assert(p.y_to_pitch(p.pitch_to_y(60))==60);
  assert(p.x_to_beat(p.beat_to_x(4))==4);
  assert(p.snap(1.4,0.5)==1.5);
  rejects([&]{p.snap(1,0);});
  p.erase("note1");assert(p.notes().size()==1);assert(p.undo());assert(p.notes().size()==2);
  assert(p.redo());assert(p.notes().size()==1);
  return 0;
}
