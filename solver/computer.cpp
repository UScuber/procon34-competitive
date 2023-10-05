#include <time.h>
#include "base.hpp"
#include "tsp.hpp"

struct Game {
  Field field;
  Walls build_walls;
  Game(const Field &f) : field(f){}
  void run(){
    assert(field.is_my_turn());
    const auto &current_agents = field.get_now_turn_agents();
    cerr << "run\n";

    Actions res = calculate_build_route(build_walls, field);
    const int m = res.size();
    std::vector<int> dirs(m);
    std::vector<std::string> cmd(m, "none");
    for(int i = 0; i < m; i++){
      assert(i == res[i].agent_idx);
      for(int d = 0; d < 8; d++){
        if(current_agents[i] + dmove[d] == res[i].pos){
          dirs[i] = d;
          break;
        }
      }
    }
    field.update_turn_and_fix_actions(res);
    for(int i = 0; i < m; i++){
      if(res[i].command == Action::Move) cmd[i] = "move";
      if(res[i].command == Action::Build) cmd[i] = "build";
      if(res[i].command == Action::Break) cmd[i] = "break";
    }
    field.debug();
    cerr << "my turn\n";
    for(int i = 0; i < m; i++){
      std::cout << dirs[i] << " " << cmd[i] << "\n";
      cerr << dirs[i] << " " << cmd[i] << "\n";
    }
  }

  void load(){
    assert(!field.is_my_turn());
    const auto &current_agents = field.get_now_turn_agents();
    std::vector<Action> res;
    cerr << "enemy turn\n";
    for(int i = 0; i < (int)current_agents.size(); i++){
      int dir; std::string str;
      std::cin >> dir >> str;
      cerr << dir << " " << str << "\n";
      uchar cmd = Action::None;
      if(str == "move") cmd = Action::Move;
      if(str == "build") cmd = Action::Build;
      if(str == "break") cmd = Action::Break;
      res.emplace_back(Action(current_agents[i] + dmove[dir], cmd, i));
    }
    build_walls.clear();
    int walls_num;
    std::cin >> walls_num;
    for(int i = 0; i < walls_num; i++){
      Point p;
      std::cin >> p;
      build_walls.push_back(p);
    }
    field.update_turn(res);
    field.debug();
  }
};



int main(){
  srand(time(NULL));
  std::cin >> height >> width;
  Field field = read_field(height, width);
  field.debug();

  Game game(field);
  while(!game.field.is_finished()){
    if(game.field.is_my_turn()){
      StopWatch sw;
      game.run();
      cerr << "Elapsed Time: " << sw.get_ms() << "[ms]\n";
    }else{
      game.load();
    }
  }
  cerr << "Finished\n";
}
