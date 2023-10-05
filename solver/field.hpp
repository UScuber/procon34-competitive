#pragma once

#include <queue>
#include <map>
#include "lib.hpp"


struct Action {
  static constexpr uchar None = 0;
  static constexpr uchar Break = 1;
  static constexpr uchar Build = 2;
  static constexpr uchar Move = 3;

  uchar command; int agent_idx;
  Point pos;

  // posは移動先を表す、構築/破壊はその対象の場所がposになる
  inline constexpr Action(const Point &_p, const uchar _cmd, const int _agent_idx=-1) : pos(_p), command(_cmd), agent_idx(_agent_idx){
    assert(0 <= command && command < 4);
  }
  inline constexpr bool operator<(const Action &act) const{
    if(command != act.command) return command < act.command;
    return pos < act.pos;
  }
  inline constexpr bool operator==(const Action &act) const{
    return command == act.command && agent_idx == act.agent_idx && pos == act.pos;
  }
};

struct Agent : Point {
  inline constexpr Agent(const Pos y=-1, const Pos x=-1) : Point(y, x){}
  inline constexpr Agent(const Point p) : Point(p){}
};

struct Wall : Point {
  inline constexpr Wall(const Pos y=-1, const Pos x=-1) : Point(y, x){}
  inline constexpr Wall(const Point p) : Point(p){}
};


using Actions = std::vector<Action>;
using Agents = std::vector<Agent>;
using Walls = std::vector<Wall>;

struct Field {

  std::vector<std::vector<State>> field;
  Agents ally_agents, enemy_agents;
  std::vector<Point> castles;
  int side, current_turn, final_turn, TL;

  Field(const int h, const int w,
        const std::vector<Point> &ponds,
        const std::vector<Point> &_castles,
        const Agents &_ally_agents,
        const Agents &_enemy_agents,
        const int _side, // 0 or 1
        const int _final_turn,
        const int _TL)
    : field(h, std::vector<State>(w, State::None)),
      ally_agents(_ally_agents),
      enemy_agents(_enemy_agents),
      castles(_castles),
      side(_side),
      current_turn(0),
      final_turn(_final_turn),
      TL(_TL){
    assert(ally_agents.size() == enemy_agents.size()); // check

    for(const auto &p : ponds) field[p.y][p.x] |= State::Pond;
    for(const auto &p : castles) field[p.y][p.x] |= State::Castle;
    for(const auto &p : ally_agents) field[p.y][p.x] |= State::Ally;
    for(const auto &p : enemy_agents) field[p.y][p.x] |= State::Enemy;
  }
  
  inline State get_state(const int y, const int x) const noexcept{
    assert(is_valid(y, x));
    return field[y][x];
  }
  inline State get_state(const Point &p) const noexcept{
    assert(is_valid(p.y, p.x));
    return get_state(p.y, p.x);
  }
  inline void set_state(const int y, const int x, const State state) noexcept{
    assert(is_valid(y, x));
    field[y][x] = state;
  }
  inline void set_state(const Point &p, const State state) noexcept{ set_state(p.y, p.x, state); }

  // スコア計算
  int calc_final_score() const{
    int ally_walls = 0, enemy_walls = 0;
    int ally_area = 0, enemy_area = 0;
    int allys_castle = 0, enemys_castle = 0;
    // 領域計算
    for(int i = 0; i < height; i++){
      for(int j = 0; j < width; j++){
        const State s = get_state(i, j);
        if(s & State::WallAlly) ally_walls++;
        if(s & State::WallEnemy) enemy_walls++;
        if(s & State::AreaAlly) ally_area++;
        if(s & State::AreaEnemy) enemy_area++;
        if(s & State::Castle){
          if(s & State::AreaAlly) allys_castle++;
          if(s & State::AreaEnemy) enemys_castle++;
        }
      }
    }
    const int score = (ally_walls-enemy_walls)*wall_coef + (ally_area-enemy_area)*area_coef + (allys_castle-enemys_castle)*castles_coef;
    return score;
  }

  // 領地の更新
  void update_region(){
    static constexpr uchar NotSeen = 0;
    static constexpr uchar Area = 1;
    static constexpr uchar Neutral = 2;

    static std::queue<Point> que;

    auto calc_region = [&](const State my_wall) -> std::vector<std::vector<uchar>> {
      std::vector<std::vector<uchar>> used(height, std::vector<uchar>(width, NotSeen));

      // fill Neutral
      for(int i = 0; i < height; i++){
        if(!(get_state(i, 0) & my_wall)){
          que.push(Point(i, 0));
          used[i][0] = Neutral;
        }
        if(!(get_state(i, width-1) & my_wall)){
          que.push(Point(i, width-1));
          used[i][width-1] = Neutral;
        }
      }
      for(int j = 0; j < width; j++){
        if(!(get_state(0, j) & my_wall)){
          que.push(Point(0, j));
          used[0][j] = Neutral;
        }
        if(!(get_state(height-1, j) & my_wall)){
          que.push(Point(height-1, j));
          used[height-1][j] = Neutral;
        }
      }
      while(!que.empty()){
        const Point pos = que.front();
        que.pop();
        for(int dir = 0; dir < 4; dir++){
          const Point nxt = pos + dmove[dir];
          if(!is_valid(nxt)) continue;
          if(used[nxt.y][nxt.x] == NotSeen && !(get_state(nxt) & my_wall)){
            used[nxt.y][nxt.x] = Neutral;
            que.push(nxt);
          }
        }
      }
      // fill ally or enemy 's area
      for(int i = 1; i < height-1; i++){
        for(int j = 1; j < width-1; j++){
          if(used[i][j] == NotSeen && !(get_state(i, j) & my_wall)){
            used[i][j] = Area;
          }
        }
      }
      return used;
    };

    const auto ally_reg = calc_region(State::WallAlly);
    const auto enemy_reg = calc_region(State::WallEnemy);
    
    // 片方囲われている -> 外された <-- そのまま
    // 片方囲われている -> 片方に   <-- その片方に
    // 両方囲われている -> 外された <-- そのまま(まだ不明)
    // 両方囲われている -> 片方に   <-- その片方に
    //           |
    //           v
    // 外されている -> そのまま
    // 片方になる   -> その片方に
    for(int i = 0; i < height; i++){
      for(int j = 0; j < width; j++){
        const State st = get_state(i, j);
        if(ally_reg[i][j] == Area && enemy_reg[i][j] == Area){
          set_state(i, j, st | State::Area);
        }else if(ally_reg[i][j] == Area){
          set_state(i, j, (st | State::AreaAlly) & ~State::AreaEnemy);
        }else if(enemy_reg[i][j] == Area){
          set_state(i, j, (st | State::AreaEnemy) & ~State::AreaAlly);
        }
        if(st & State::WallAlly) set_state(i, j, st & ~State::AreaAlly);
        if(st & State::WallEnemy) set_state(i, j, st & ~State::AreaEnemy);
      }
    }
  }

  // side: 味方:0, 敵:1
  void update_field(const Actions &acts){
    assert(acts.size() == ally_agents.size());
    Actions act_list[4];
    std::map<Point, int> agent_poses;

    for(const auto &act : acts){
      act_list[act.command].emplace_back(act);
      assert(0 <= act.agent_idx && act.agent_idx < (int)acts.size());
      if(act.command == Action::Move) agent_poses[act.pos]++;
    }
    
    // ally turn
    if(!(current_turn & 1)){
      for(const Point agent : ally_agents) agent_poses[agent]++;
      // break
      for(const auto &act : act_list[Action::Break]){
        const State st = get_state(act.pos);
        if(!(st & State::Wall)){
          cerr << "Error: there is not wall at(" << act.pos << ")\n";
          continue;
        }
        set_state(act.pos, st & ~State::Wall);
      }
      // build
      for(const auto &act : act_list[Action::Build]){
        const State st = get_state(act.pos);
        if(st & (State::WallEnemy | State::Enemy | State::Castle)){
          cerr << "Error: there is wallenemy, enemy or castle at(" << act.pos << ")\n";
          continue;
        }
        if(st & State::WallAlly){ // someone already built
          cerr << "Error: someone has already built on(" << act.pos << ")\n";
          continue;
        }
        set_state(act.pos, st | State::WallAlly);
      }
      // move
      for(const auto &act : act_list[Action::Move]){
        const State st = get_state(act.pos);
        if(st & (State::Human | State::Pond | State::WallEnemy)){
          cerr << "Error: there is human, pond or wallenemy at(" << act.pos << ")\n";
          continue;
        }
        if(agent_poses[act.pos] >= 2){
          cerr << "Error: many humans at(" << act.pos << ")\n";
          continue;
        }
        const auto from = ally_agents[act.agent_idx];
        set_state(from, get_state(from) ^ State::Ally);
        set_state(act.pos, st | State::Ally);
        ally_agents[act.agent_idx] = act.pos;
      }
    }
    // enemy turn
    else{
      for(const Point agent : enemy_agents) agent_poses[agent]++;
      // break
      for(const auto &act : act_list[Action::Break]){
        const State st = get_state(act.pos);
        if(!(st & State::Wall)){
          cerr << "Error: there is not wall at(" << act.pos << ")\n";
          continue;
        }
        set_state(act.pos, st & ~State::Wall);
      }
      // build
      for(const auto &act : act_list[Action::Build]){
        const State st = get_state(act.pos);
        if(st & (State::WallAlly | State::Ally | State::Castle)){
          cerr << "Error: there is wallally, ally or castle at(" << act.pos << ")\n";
          continue;
        }
        if(st & State::WallEnemy){ // someone already built
          cerr << "Error: someone has already built on(" << act.pos << ")\n";
          continue;
        }
        set_state(act.pos, st | State::WallEnemy);
      }
      // move
      for(const auto &act : act_list[Action::Move]){
        const State st = get_state(act.pos);
        if(st & (State::Human | State::Pond | State::WallAlly)){
          cerr << "Error: there is human, pond or wallally at(" << act.pos << ")\n";
          continue;
        }
        if(agent_poses[act.pos] >= 2){
          cerr << "Error: many humans at(" << act.pos << ")\n";
          continue;
        }
        const auto from = enemy_agents[act.agent_idx];
        set_state(from, get_state(from) ^ State::Enemy);
        set_state(act.pos, st | State::Enemy);
        enemy_agents[act.agent_idx] = act.pos;
      }
    }
    update_region();
  }

  // side: 味方:0, 敵:1
  void update_field_and_fix_actions(Actions &acts){
    assert(acts.size() == ally_agents.size());
    std::vector<Action*> act_list[4];
    std::map<Point, int> agent_poses;

    for(auto &act : acts){
      act_list[act.command].emplace_back(&act);
      assert(0 <= act.agent_idx && act.agent_idx < (int)acts.size());
      if(act.command == Action::Move) agent_poses[act.pos]++;
    }
    
    // ally turn
    if(!(current_turn & 1)){
      for(const Point agent : ally_agents) agent_poses[agent]++;
      // break
      for(auto *act : act_list[Action::Break]){
        const State st = get_state(act->pos);
        if(!(st & State::Wall)){
          cerr << "Error: there is not wall at(" << act->pos << ")\n";
          act->command = Action::None;
          continue;
        }
        set_state(act->pos, st & ~State::Wall);
      }
      // build
      for(auto *act : act_list[Action::Build]){
        const State st = get_state(act->pos);
        if(st & (State::WallEnemy | State::Enemy | State::Castle)){
          cerr << "Error: there is wallenemy, enemy or castle at(" << act->pos << ")\n";
          act->command = Action::None;
          continue;
        }
        if(st & State::WallAlly){ // someone already built
          cerr << "Error: someone has already built on(" << act->pos << ")\n";
          act->command = Action::None;
          continue;
        }
        set_state(act->pos, st | State::WallAlly);
      }
      // move
      for(auto *act : act_list[Action::Move]){
        const State st = get_state(act->pos);
        if(st & (State::Human | State::Pond | State::WallEnemy)){
          cerr << "Error: there is human, pond or wallenemy at(" << act->pos << ")\n";
          act->command = Action::None;
          continue;
        }
        if(agent_poses[act->pos] >= 2){
          cerr << "Error: many humans at(" << act->pos << ")\n";
          act->command = Action::None;
          continue;
        }
        const auto from = ally_agents[act->agent_idx];
        set_state(from, get_state(from) ^ State::Ally);
        set_state(act->pos, st | State::Ally);
        ally_agents[act->agent_idx] = act->pos;
      }
    }
    // enemy turn
    else{
      for(const Point agent : enemy_agents) agent_poses[agent]++;
      // break
      for(auto *act : act_list[Action::Break]){
        const State st = get_state(act->pos);
        if(!(st & State::Wall)){
          cerr << "Error: there is not wall at(" << act->pos << ")\n";
          act->command = Action::None;
          continue;
        }
        set_state(act->pos, st & ~State::Wall);
      }
      // build
      for(auto *act : act_list[Action::Build]){
        const State st = get_state(act->pos);
        if(st & (State::WallAlly | State::Ally | State::Castle)){
          cerr << "Error: there is wallally, ally or castle at(" << act->pos << ")\n";
          act->command = Action::None;
          continue;
        }
        if(st & State::WallEnemy){ // someone already built
          cerr << "Error: someone has already built on(" << act->pos << ")\n";
          act->command = Action::None;
          continue;
        }
        set_state(act->pos, st | State::WallEnemy);
      }
      // move
      for(auto *act : act_list[Action::Move]){
        const State st = get_state(act->pos);
        if(st & (State::Human | State::Pond | State::WallAlly)){
          cerr << "Error: there is human, pond or wallally at(" << act->pos << ")\n";
          act->command = Action::None;
          continue;
        }
        if(agent_poses[act->pos] >= 2){
          cerr << "Error: many humans at(" << act->pos << ")\n";
          act->command = Action::None;
          continue;
        }
        const auto from = enemy_agents[act->agent_idx];
        set_state(from, get_state(from) ^ State::Enemy);
        set_state(act->pos, st | State::Enemy);
        enemy_agents[act->agent_idx] = act->pos;
      }
    }
    update_region();
  }

  // s: 味方:0, 敵:1
  bool is_legal_action(const Actions &acts, int s = -1) const{
    if(s == -1) s = side;
    assert(acts.size() == ally_agents.size());
    Actions act_list[4];
    std::map<Point, int> agent_poses;

    for(const auto &act : acts){
      act_list[act.command].emplace_back(act);
      assert(0 <= act.agent_idx && act.agent_idx < (int)acts.size());
      if(act.command == Action::Move) agent_poses[act.pos]++;
    }
    
    // ally turn
    if(!side){
      for(const Point agent : ally_agents) agent_poses[agent]++;
      // break
      for(const auto &act : act_list[Action::Break]){
        const State st = get_state(act.pos);
        if(!(st & State::Wall)) return false;
      }
      // build
      for(const auto &act : act_list[Action::Build]){
        const State st = get_state(act.pos);
        if(st & (State::WallEnemy | State::Enemy | State::Castle)) return false;
        if(st & State::WallAlly){ // someone already built
          continue;
        }
      }
      // move
      for(const auto &act : act_list[Action::Move]){
        const State st = get_state(act.pos);
        if(st & (State::Human | State::Pond | State::WallEnemy)) return false;
        if(agent_poses[act.pos] >= 2) return false;
      }
    }
    // enemy turn
    else{
      for(const Point agent : enemy_agents) agent_poses[agent]++;
      // break
      for(const auto &act : act_list[Action::Break]){
        const State st = get_state(act.pos);
        if(!(st & State::Wall)) return false;
      }
      // build
      for(const auto &act : act_list[Action::Build]){
        const State st = get_state(act.pos);
        if(st & (State::WallAlly | State::Ally | State::Castle)) return false;
        if(st & State::WallEnemy){ // someone already built
          continue;
        }
      }
      // move
      for(const auto &act : act_list[Action::Move]){
        const State st = get_state(act.pos);
        if(st & (State::Human | State::Pond | State::WallAlly)) return false;
        if(agent_poses[act.pos] >= 2) return false;
      }
    }
    return true;
  }

  void update_turn(const Actions &acts){
    update_field(acts);
    current_turn++;
  }
  void update_turn_and_fix_actions(Actions &acts){
    update_field_and_fix_actions(acts);
    current_turn++;
  }
  Agents &get_now_turn_agents(){
    if(current_turn & 1) return enemy_agents;
    return ally_agents;
  }
  const Agents &get_now_turn_agents() const{
    if(current_turn & 1) return enemy_agents;
    return ally_agents;
  }
  bool is_finished() const{ return current_turn == final_turn; }
  bool is_my_turn() const{ return (current_turn & 1) == side; }
  void debug() const{
    std::vector<std::string> board(height), wall(height), region(height);
    for(int i = 0; i < height; i++){
      for(int j = 0; j < width; j++){
        const State s = get_state(i, j);
        //  board
        char c = '.';
        if(s & State::Pond) c = '#';
        else if(s & State::Ally) c = '@';
        else if(s & State::Enemy) c = '%';
        else if(s & State::Castle) c = '$';
        board[i] += c;
        // wall
        c = '.';
        if((s & State::WallAlly) && (s & State::WallEnemy)) c = '$';
        else if(s & State::WallAlly) c = '@';
        else if(s & State::WallEnemy) c = '%';
        else if(s & State::Castle) c = '$';
        wall[i] += c;
        // region
        c = '.';
        if((s & State::AreaAlly) && (s & State::AreaEnemy)) c = '$';
        else if(s & State::AreaAlly) c = '@';
        else if(s & State::AreaEnemy) c = '%';
        else if(s & State::Castle) c = '$';
        region[i] += c;
      }
    }
    cerr << "board" << std::string(width-4, ' ') << ": walls" << std::string(width-4, ' ') << ": region\n";
    for(int i = 0; i < height; i++){
      cerr << board[i] << " : " << wall[i] << " : " << region[i] << "\n";
    }
    cerr << "\n";
  }
};


Field read_field(const int h, const int w){
  auto get_points = []() -> std::vector<Point> {
    int num;
    std::cin >> num;
    assert(0 <= num);
    std::vector<Point> res(num);
    for(int i = 0; i < num; i++) std::cin >> res[i];
    return res;
  };

  int side; // 先行:0,後攻:1
  int final_turn, TL;
  std::cin >> side >> final_turn >> TL;
  assert(side == 0 || side == 1);
  const auto ponds = get_points();
  const auto castles = get_points();
  Agents ally_agents, enemy_agents;
  for(const Point p : get_points()) ally_agents.emplace_back(p);
  for(const Point p : get_points()) enemy_agents.emplace_back(p);
  return Field(
    h, w,
    ponds,
    castles,
    ally_agents,
    enemy_agents,
    side,
    final_turn,
    TL
  );
}
