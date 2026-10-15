#include "../src/term.hpp"
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include <deque>
namespace term {
static int g_w=118,g_h=40; static std::deque<KeyEvent> g_keys; static bool g_init=false;
static std::vector<std::string> g_lastFrame;
std::wstring widen(const std::string&s){return std::wstring(s.begin(),s.end());}
std::string narrow(const std::wstring&w){return std::string(w.begin(),w.end());}
bool isWindowsTerminal(){return true;}
bool init(){ g_init=true;
  // sequência de teclas via env USBSCOPE_KEYS, ex "DDD\nENTER\nTAB\nESC\nq"
  const char* s=getenv("USBSCOPE_KEYS"); std::string k=s?s:"q";
  for(size_t i=0;i<k.size();++i){ char c=k[i];
    if(c=='D')g_keys.push_back({Key::Down,0,0});
    else if(c=='U')g_keys.push_back({Key::Up,0,0});
    else if(c=='L')g_keys.push_back({Key::Left,0,0});
    else if(c=='R')g_keys.push_back({Key::Right,0,0});
    else if(c=='T')g_keys.push_back({Key::Tab,0,0});
    else if(c=='E')g_keys.push_back({Key::Enter,0,0});
    else if(c=='X')g_keys.push_back({Key::Escape,0,0});
    else g_keys.push_back({Key::Char,(char32_t)c,(char32_t)c});
  }
  return true;
}
void shutdown(){ for(auto&l:g_lastFrame) printf("%s\x1b[0m\n",l.c_str()); }
int width(){return g_w;} int height(){return g_h;}
void write(const std::string&){}
void present(const std::vector<std::string>&lines){ g_lastFrame=lines; }
KeyEvent readKey(int){ if(g_keys.empty()){ shutdown(); exit(0);} 
  KeyEvent e=g_keys.front(); g_keys.pop_front(); return e; }
std::string readLine(const std::string&){ const char* f=getenv("USBSCOPE_FILTER"); return f?f:""; }
void setTitle(const std::string&){}
void sleepMs(int){}
}
