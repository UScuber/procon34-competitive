#pragma once

#include <set>
#include "field.hpp"

Actions enumerate_next_agent_acts(const Agent &agent, const Field &field, const bool use_assert=true){
  const State ally = field.get_state(agent) & State::Human; // agentから見た味方
  const State enemy = ally ^ State::Human; // agentから見た敵
  if(use_assert) assert((ally == State::Enemy) == (field.current_turn & 1));
  if(use_assert) assert(ally == State::Ally || ally == State::Enemy);

  const State ally_wall = ally == State::Ally ? State::WallAlly : State::WallEnemy; // agentから見た味方のwall
  const State enemy_wall = ally_wall ^ State::Wall; // agentから見た敵のwall

  Actions actions;
  for(int dir = 0; dir < 8; dir++){
    const auto nxt = agent + dmove[dir];
    if(!is_valid(nxt)) continue;
    const State st = field.get_state(nxt);
    if(dir < 4){
      // break (自陣の壁の破壊は考慮しない)
      if(st & enemy_wall) actions.emplace_back(Action(nxt, Action::Break));
      // build
      if(!(st & (State::Wall | State::Castle | enemy))) actions.emplace_back(Action(nxt, Action::Build));
    }
    // can move to nxt
    if(!(st & (State::Pond | State::Human | enemy_wall))){
      actions.emplace_back(Action(nxt, Action::Move));
    }
  }
  return actions;
}


// 適当に選ぶ
Actions select_random_next_agents_acts(const Agents &agents, const Field &field){
  Actions result;
  std::set<Action> cnt;
  for(const auto &agent : agents){
    assert(((field.get_state(agent) & State::Human) == State::Enemy) == (field.current_turn & 1));
    auto acts = enumerate_next_agent_acts(agent, field);
    if(acts.empty()) acts.emplace_back(Action(agent, Action::None));
    int num = 0;
    int idx = rnd(acts.size());
    while(num++ < 10 && cnt.count(acts[idx])) idx = rnd(acts.size());
    if(num >= 10){
      acts.emplace_back(Action(agent, Action::None));
      idx = (int)acts.size() - 1;
    }
    cnt.insert(acts[idx]);
    result.emplace_back(acts[idx]);
    result.back().agent_idx = (int)result.size() - 1;
  }
  return result;
}

#include <algorithm>
// 後でちゃんと書きます...
std::vector<Actions> enumerate_next_all_agents_acts(const Agents &agents, const Field &field){
  std::vector<Actions> acts;
  for(int i = 0; i < 100; i++){
    acts.emplace_back(select_random_next_agents_acts(agents, field));
  }
  std::sort(acts.begin(), acts.end());
  acts.erase(std::unique(acts.begin(), acts.end()), acts.end());
  return acts;
}


namespace Evaluate {

int calc_agent_min_dist(const Field &field, const Agents &ally_agents, const State area){
  int dc = 0;
  for(const Agent agent : ally_agents){
    int min_dist = 1000;
    for(const auto castle : field.castles){
      if((field.get_state(castle) & State::Area) == area) continue;
      const int d = manche_dist(agent, castle);
      if(min_dist > d) min_dist = d;
    }
    dc += min_dist * min_dist;
  }
  return dc;
}

int calc_wall_min_dist(const Field &field, const State wall){
  int dw = 0;
  for(int i = 0; i < height; i++){
    for(int j = 0; j < width; j++) if(field.get_state(i, j) & wall){
      int min_dist = 1000;
      for(const auto castle : field.castles){
        const int d = manche_dist(Point(i, j), castle);
        if(min_dist > d) min_dist = d;
      }
      dw += min_dist * min_dist;
    }
  }
  return dw;
}

template <class F>
std::vector<std::vector<int>> calc_castle_min_dist(const Field &field, const F &dist){
  std::vector<std::vector<int>> res(height, std::vector<int>(width));
  for(int i = 0; i < height; i++){
    for(int j = 0; j < width; j++){
      int min_dist = 1000;
      for(const auto castle : field.castles){
        const int d = dist(Point(i, j), castle);
        if(min_dist > d) min_dist = d;
      }
      res[i][j] = min_dist;
    }
  }
  return res;
}

double calc_around_wall(const Field &field, const State wall){
  static constexpr int C = 4;
  static std::vector<std::vector<int>> dist_table = calc_castle_min_dist(field, manche_dist);
  int wall_num = 0, mass = 0;
  for(int i = 0; i < height; i++){
    for(int j = 0; j < width; j++) if(dist_table[i][j] <= C){
      if(field.get_state(i, j) & wall) wall_num++;
      mass++;
    }
  }
  return (double)wall_num / mass;
}

double calc_nearest_wall(const Field &field, const State wall){
  static std::vector<std::vector<int>> dist_table = calc_castle_min_dist(field, manche_dist);
  double res = 0;
  for(int i = 0; i < height; i++){
    for(int j = 0; j < width; j++) if(field.get_state(i, j) & wall){
      res += 1.0 / (dist_table[i][j] * dist_table[i][j]);
    }
  }
  return res;
}

int calc_connected_wall(const Field &field, const State wall){
  static std::queue<Point> que;
  static std::vector<std::vector<int>> used(height, std::vector<int>(width));
  static int unused = 0;
  int res = 0;
  for(int i = 0; i < height; i++){
    for(int j = 0; j < width; j++){
      if((field.get_state(i, j) & wall) && used[i][j] <= unused){
        int num = 0;
        used[i][j] = unused + 1;
        que.push(Point(i, j));
        while(!que.empty()){
          const auto pos = que.front();
          que.pop();
          num++;
          for(int dir = 0; dir < 8; dir++){
            const auto nxt = pos + dmove[dir];
            if(!is_valid(nxt) || !(field.get_state(nxt) & wall)) continue;
            if(used[nxt.y][nxt.x] <= unused){
              used[nxt.y][nxt.x] = unused + 1;
              que.push(nxt);
            }
          }
        }
        const int lim = std::min(num, 5);
        res += lim*lim + num-lim;
      }
    }
  }
  unused++;
  return res;
}

int calc_wall_by_enemy(const Field &field, const Agents &enemy_agents, const State wall){
  int res = 0;
  for(const auto agent : enemy_agents){
    for(int dir = 0; dir < 4; dir++){
      const auto nxt = agent + dmove[dir];
      if(!is_valid(nxt)) continue;
      if(field.get_state(nxt) & wall) res++;
    }
  }
  return res;
}

double evaluate_field(const Field &field){
  /*
  // eval #1: 各職人から一番近い城までの距離^2の総和
  const int dc = calc_agent_min_dist(field, field.ally_agents, State::AreaAlly) - calc_agent_min_dist(field, field.enemy_agents, State::AreaEnemy);
  // eval #2: 各城壁から一番近い城との距離の総和
  const int dw = calc_wall_min_dist(field, State::WallAlly) - calc_wall_min_dist(field, State::WallEnemy);
  // eval #3: 各城を中心として、((距離がC以内にある城壁の個数)/(対象のマスの数))
  const double pw = calc_around_wall(field, State::WallAlly) - calc_around_wall(field, State::WallEnemy);
  // eval #4: 1/(壁から一番近い城までの距離^2)の総和
  const double wd = calc_nearest_wall(field, State::WallAlly) - calc_nearest_wall(field, State::WallEnemy);
  // eval #5: 城壁の各連結成分の大きさ^2の総和
  const int w = calc_connected_wall(field, State::WallAlly) - calc_connected_wall(field, State::WallEnemy);
  // eval #6: 城、領域、壁の数
  const int n = field.calc_final_score();
  // eval #7: 敵の職人のマンハッタン距離1以内に置かれている壁の数
  const int wn = calc_wall_by_enemy(field, field.enemy_agents, State::WallAlly) - calc_wall_by_enemy(field, field.ally_agents, State::WallEnemy);

  static constexpr double a = 0.0015 * 0;
  static constexpr double b = 0.010 * 0;
  static constexpr double c = 0.95 * 0;
  static constexpr double d = 1.3 * 0;
  static constexpr double e = 0.01;
  static constexpr double f = 0.07 * 0.01;
  double res = 0;
  res -= dc * a;
  res -= dw * b;
  res += pw * c;
  res += wd * d;
  res += w * e;
  res += n * 0.1;
  res -= wn * f;
  return res * 0.5;
  */
  return field.calc_final_score() * 0.1 * 0.5;
}

double evaluate_field2(const Field &field){
  // eval #1: 各職人から一番近い城までの距離^2の総和
  const int dc = calc_agent_min_dist(field, field.ally_agents, State::AreaAlly) - calc_agent_min_dist(field, field.enemy_agents, State::AreaEnemy);
  // eval #2: 各城壁から一番近い城との距離の総和
  const int dw = calc_wall_min_dist(field, State::WallAlly) - calc_wall_min_dist(field, State::WallEnemy);
  // eval #3: 各城を中心として、((距離がC以内にある城壁の個数)/(対象のマスの数))
  const double pw = calc_around_wall(field, State::WallAlly) - calc_around_wall(field, State::WallEnemy);
  // eval #4: 1/(壁から一番近い城までの距離^2)の総和
  const double wd = calc_nearest_wall(field, State::WallAlly) - calc_nearest_wall(field, State::WallEnemy);
  // eval #5: 城壁の各連結成分の大きさ^2の総和
  const int w = calc_connected_wall(field, State::WallAlly) - calc_connected_wall(field, State::WallEnemy);
  // eval #6: 城、領域、壁の数
  const int n = field.calc_final_score();
  // eval #7: 敵の職人のマンハッタン距離1以内に置かれている壁の数
  const int wn = calc_wall_by_enemy(field, field.enemy_agents, State::WallAlly) - calc_wall_by_enemy(field, field.ally_agents, State::WallEnemy);

  static constexpr double a = 0.004;
  static constexpr double b = 0.007;
  static constexpr double c = 0.65;
  static constexpr double d = 1.2;
  static constexpr double e = 0.001;
  static constexpr double f = 0.07;
  double res = 0;
  res -= dc * a;
  res -= dw * b;
  res += pw * c;
  res += wd * d;
  res += w * e;
  res += n * 0.1;
  res -= wn * f;
  return res * 0.5;
}

}
