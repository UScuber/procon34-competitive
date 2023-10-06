#pragma once

#include <vector>
#include <iostream>
#include <algorithm>
#include <cstdint>

using uchar = unsigned char;
using uint = unsigned int;
using ull = unsigned long long;
using ll = long long;
using Pos = uint8_t; // y,x座標の型

constexpr int max_height = 25;
constexpr int max_width = 25;
constexpr int max_agent_num = 6;
constexpr int castles_coef = 10, area_coef = 3, wall_coef = 1;

constexpr Pos dy[] = { (Pos)-1,0,1,0, (Pos)-1,1,1,(Pos)-1 };
constexpr Pos dx[] = { 0,(Pos)-1,0,1, (Pos)-1,(Pos)-1,1,1 };

int height = 0, width = 0; // fieldの大きさ

// error出力
#ifndef NOERRFILE
  #include <fstream>
  struct Cerr {
    Cerr(const std::string &filename) : os(filename){}
    template <class T>
    inline Cerr &operator<<(const T &val) noexcept{
      os << val << std::flush;
      return *this;
    }
  private:
    std::ofstream os;
  };
  Cerr cerr("stderr.txt");
#else
  using std::cerr;
#endif


// stacktrace/
#include <sstream>
#if defined(__linux__) || defined(__APPLE__) || defined(__FreeBSD__)
  #define __posix__
  #include <execinfo.h>
#elif defined(__MINGW32__)
  #include <windows.h>
#endif

std::string get_backtrace_info(){
  std::string result;
#if defined(__posix__)
  static constexpr ll trace_frames_max = 100;
  void *trace_frames[trace_frames_max] = {};
  const int size = ::backtrace(trace_frames, trace_frames_max);
  if(size < 0 || size >= trace_frames_max){
    return "get_backtrace_info error: backtrace failed";
  }
  char **trace_symbols = ::backtrace_symbols(trace_frames, trace_frames_max);
  if(!trace_symbols){
    return "get_backtrace_info error: get symbol failed";
  }
  for(ll i = 1; i < size; i++){
    result.append(trace_symbols[i]);
    result.append("\n");
  }
  free(trace_symbols);
  trace_symbols = nullptr;
#elif defined(__MINGW32__)
  static constexpr ll trace_frames_max = 50;
  void *trace_frames[trace_frames_max] = {};
  const int size = ::CaptureStackBackTrace(0, trace_frames_max, trace_frames, nullptr);
  if(size < 0 || size >= trace_frames_max){
    return "get_backtrace_info error: backtrace failed";
  }
  std::ostringstream address_str;
  for(ll i = 1; i < size; i++){
    const ll address = reinterpret_cast<ll>(trace_frames[i]);
    address_str << i << ": 0x" << std::hex << address << "\n";
  }
  result.assign(address_str.str());
#else
  return "get_backtrace_info error: unknow platform";
#endif
  return result;
}

void _assertion_failed(char const *expr){
  const auto info = get_backtrace_info();
  cerr << "Expression '" << expr << "' is false in\n" << info << "\n";
  std::abort();
}

#ifndef NDEBUG
  #define assert(expr) \
  (void) \
  ((!!(expr)) || \
  (cerr << "Assertion Failed!!! " << __FILE__ << ", " << __LINE__ << "\n", \
  _assertion_failed(#expr), 0))
#else
  #define assert(expr)
#endif



struct State {
  static const State None;
  static const State Pond;
  static const State WallEnemy;
  static const State WallAlly;
  static const State AreaEnemy;
  static const State AreaAlly;
  static const State Enemy; // 敵のagent
  static const State Ally; // 味方のagent
  static const State Castle;
  static const State Human; // Ally | Enemey
  static const State Wall; // WallAlly | WallEnemy
  static const State Area; // AreaAlly | AreaEnemy

  inline constexpr State(const uchar v=0) : val(v){}
  inline constexpr State &operator|=(const State s) noexcept{ val |= s.val; return *this; }
  inline constexpr State &operator&=(const State s) noexcept{ val &= s.val; return *this; }
  inline constexpr State &operator^=(const State s) noexcept{ val ^= s.val; return *this; }
  inline constexpr State operator|(const State s) const noexcept{ return State(val | s.val); }
  inline constexpr State operator&(const State s) const noexcept{ return State(val & s.val); }
  inline constexpr State operator^(const State s) const noexcept{ return State(val ^ s.val); }
  inline constexpr State operator~() const noexcept{ return State(~val); }
  inline constexpr operator bool() const noexcept{ return val; }
  inline constexpr bool operator==(const State s) const noexcept{ return val == s.val; }
  friend std::ostream &operator<<(std::ostream &os, const State s){ return os << (int)s.val; }

protected:
  uchar val;
};
constexpr State State::None = 0;
constexpr State State::Pond = 1 << 0;
constexpr State State::WallEnemy = 1 << 1;
constexpr State State::WallAlly = 1 << 2;
constexpr State State::AreaEnemy = 1 << 3;
constexpr State State::AreaAlly = 1 << 4;
constexpr State State::Enemy = 1 << 5;
constexpr State State::Ally = 1 << 6;
constexpr State State::Castle = 1 << 7;
constexpr State State::Human = State::Ally | State::Enemy;
constexpr State State::Wall = State::WallAlly | State::WallEnemy;
constexpr State State::Area = State::AreaAlly | State::AreaEnemy;


struct Point {
  Pos y,x;
  inline constexpr Point(const Pos _y=-1, const Pos _x=-1) : y(_y), x(_x){}
  inline constexpr Point &operator+=(const Point &p) noexcept{
    y += p.y; x += p.x; return *this;
  }
  inline constexpr Point operator+(const Point &p) const noexcept{ return Point(y + p.y, x + p.x); }
  inline constexpr bool operator<(const Point &p) const noexcept{
    if(y != p.y) return y < p.y;
    return x < p.x;
  }
  inline constexpr bool operator==(const Point &p) const noexcept{ return y == p.y && x == p.x; }
  friend std::istream &operator>>(std::istream &is, Point &p){
    int y,x;
    is >> y >> x;
    p.y = y; p.x = x;
    return is;
  }
  friend std::ostream &operator<<(std::ostream &os, const Point &p){
    return os << (int)p.y << " " << (int)p.x;
  }
};

inline constexpr bool is_valid(const Pos y, const Pos x) noexcept{
  return 0 <= y && y < (Pos)height && 0 <= x && x < (Pos)width;
}

inline constexpr bool is_valid(const Point p) noexcept{
  return 0 <= p.y && p.y < (Pos)height && 0 <= p.x && p.x < (Pos)width;
}

inline constexpr int manh_dist(const Point p, const Point q) noexcept{
  return abs(p.y - q.y) + abs(p.x - q.x);
}

inline constexpr int che_dist(const Point p, const Point q) noexcept{
  return std::max(abs(p.y - q.y), abs(p.x - q.x));
}

inline constexpr int manche_dist(const Point p, const Point q) noexcept{
  return manh_dist(p, q) + che_dist(p, q);
}

inline constexpr bool is_neighbor(const Point p, const Point q) noexcept{
  return manh_dist(p, q) == 1;
}

inline constexpr bool is_around(const Point p, const Point q) noexcept{
  return che_dist(p, q) == 1;
}

inline int to_idx(const Point p) noexcept{
  return p.y*width + p.x;
}
inline Point to_point(const int idx) noexcept{
  return Point(idx / width, idx % width);
}

template <class S, class T>
inline constexpr bool chmin(S &a, const T &b){
  return a > b ? (a = b, 1) : 0;
}
template <class S, class T>
inline constexpr bool chmax(S &a, const T &b){
  return a < b ? (a = b, 1) : 0;
}

constexpr Point dmove[] = {
  Point(-1, 0),
  Point(0, -1),
  Point(1, 0),
  Point(0, 1),
  Point(-1, -1),
  Point(1, -1),
  Point(1, 1),
  Point(-1, 1),
};


inline uint randxor32() noexcept{
  //static uint y = (uint)rand() | (uint)rand() << 16;
  static uint y = 1210253353;
  y = y ^ (y << 13); y = y ^ (y >> 17);
  return y = y ^ (y << 5);
}
// returns random [l, r)
inline int rnd(const int l, const int r) noexcept{
  return randxor32() % (r - l) + l;
}
// returns random [0, len)
inline int rnd(const int len) noexcept{
  return randxor32() % len;
}
