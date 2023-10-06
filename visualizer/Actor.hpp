# pragma once
# include <Siv3D.hpp>
# include "Field.hpp"


class Craftsman {
public:
	Craftsman(){}
	// 盤面情報に合わせて職人を配置
	Craftsman(const int y, const int x, const TEAM team);
	// 職人の行動情報を初期化
	void initialize(void);
	// 城壁の建築
	bool build(Field &field, const Point direction);
	// 城壁の破壊
	bool destroy(Field &field, const Point direction);
	// 職人の移動
	bool move(Field &field, const Point direction);
	// 行動情報の出力
	void output_act(ChildProcess &child) const;
	// 行動情報を受け取って動かす
	void input_act(ChildProcess &child, Field &field);

// private:
	// 職人の座標
	Point pos = { 0, 0 };
	// 行動済みかどうか
	bool is_acted = false;
	// 選択済みかどうか
	bool is_target = false;
	// 職人のチーム
	TEAM team = TEAM::RED;
	// 行動をした方向
	int direction = 0;
	// 行動の種類
	ACT act = ACT::NOTHING;
};

// 職人の移動範囲
const Array<Point> range_move = { {0,-1},{-1,0},{0,1},{1,0},{-1,-1},{-1,1},{1,1},{1,-1} };
// 職人の建設・破壊範囲
const Array<Point> range_wall = { {0,-1},{-1,0},{0,1},{1,0} };

// 職人の座標とチームを初期化
Craftsman::Craftsman(const int y, const int x, const TEAM team){
	this->pos = { x, y };
	this->team = team;
}

// 職人の行動情報を初期化
void Craftsman::initialize(void){
	this->is_acted = false;
	this->is_target = false;
	this->direction = 0;
	this->act = ACT::NOTHING;
}

// 建築
bool Craftsman::build(Field &field, const Point direction_point){
	const Point target_cell_pos = this->pos + direction_point;
	// 建設可能範囲内か
	if(not is_in_field(target_cell_pos) or direction_point.manhattanLength() != 1){
		return false;
	}
	const CELL target_cell = field.get_cell(target_cell_pos);
	// 建設可能な場所か
	if(target_cell & CELL::WALL or
		target_cell & switch_cell(CELL::CRAFTSMAN, not team) or
		target_cell & CELL::CASTLE){
		return false;
	}
	// フィールド変化
	field.set_bit(target_cell_pos, switch_cell(CELL::WALL, team));
	// 行動情報
	this->is_acted = true;
	this->is_target = false;
	this->act = ACT::BUILD;
	for(int i = 0; i < range_wall.size(); i++){
		if(range_wall[i] == direction_point){
			this->direction = i;
			break;
		}
	}
	return true;
}

// 破壊
bool Craftsman::destroy(Field &field, const Point direction_point){
	const Point target_cell_pos = this->pos + direction_point;
	// 破壊可能範囲内か
	if(not is_in_field(target_cell_pos) or direction_point.manhattanLength() != 1){
		return false;
	}
	const CELL target_cell = field.get_cell(target_cell_pos);
	// 破壊可能な場所か
	if(not (target_cell & CELL::WALL)){
		return false;
	}
	// フィールド変化
	field.delete_bit(target_cell_pos, CELL::WALL);
	// 行動情報
	this->is_acted = true;
	this->is_target = false;
	this->act = ACT::DESTROY;
	for (int i = 0; i < range_wall.size(); i++){
		if(range_wall[i] == direction_point){
			this->direction = i;
			break;
		}
	}
	return true;
}

// 移動
bool Craftsman::move(Field &field, const Point direction_point){
	const Point target_cell_pos = this->pos + direction_point;
	// 移動可能範囲か
	if((not is_in_field(target_cell_pos)) or Abs(direction_point.y) > 1 or Abs(direction_point.x) > 1){
		return false;
	}
	// 移動可能な場所か
	const CELL target_cell = field.get_cell(target_cell_pos);
	if((target_cell & CELL::POND) or
		(target_cell & switch_cell(CELL::WALL, not team)) or
		(target_cell & CELL::CRAFTSMAN)){
		return false;
	}
	// フィールド変化
	field.delete_bit(this->pos, switch_cell(CELL::CRAFTSMAN, team));
	this->pos = target_cell_pos;
	field.set_bit(this->pos, switch_cell(CELL::CRAFTSMAN, team));
	// 行動情報
	this->is_acted = true;
	this->is_target = false;
	this->act = ACT::MOVE;
	for (int i = 0; i < range_move.size(); i++){
		if(range_move[i] == direction_point){
			this->direction = i;
			break;
		}
	}
	return true;
}

void Craftsman::output_act(ChildProcess &child) const {
	child.ostream() << direction << std::endl;
	if(act == ACT::BUILD){
		child.ostream() << "build" << std::endl;
	}else if(act == ACT::DESTROY){
		child.ostream() << "break" << std::endl;
	}else if(act == ACT::MOVE){
		child.ostream() << "move" << std::endl;
	}else{
		child.ostream() << "none" << std::endl;
	}
}

void Craftsman::input_act(ChildProcess &child, Field &field){
	int direction_num;
	std::string act_str;
	child.istream() >> direction_num >> act_str;
	const Point direction_point = range_move[direction_num];
	if(act_str == "move"){
		this->act = ACT::MOVE;
		move(field, direction_point);
	}else if(act_str == "build"){
		this->act = ACT::BUILD;
		build(field, direction_point);
	}else if(act_str == "break"){
		this->act = ACT::DESTROY;
		destroy(field, direction_point);
	}
}
