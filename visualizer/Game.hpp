# pragma once
# include <Siv3D.hpp> // OpenSiv3D v0.6.10
# include "Base.hpp"
# include "Field.hpp"
# include "Actor.hpp"
# include "Connect.hpp"


using App = SceneManager<String, Field>;

const Array<Input> keyboard_craftsman = { Key0, Key1, Key2, Key3, Key4, Key5, Key6, Key7, Key8 };


class Game {
protected:
	// GUIによる操作
	void operate_gui(Field &field);
	// マウスクリックによる操作
	void operate_craftsman(const TEAM team, Field &field);
	// フィールドの表示
	void display_field(void) const;
	// 詳細表示
	void display_details(Field &field) const;
	// solverの初期化(引数にはsolverのチーム)
	void give_solver_initialize(const bool is_first, Field &field);
	// solverに職人の行動を渡す(引数にはsolverのチーム)
	void give_solver(const TEAM team);
	// solverから行動を受け取る(引数にはsolverのチーム)
	void receive_solver(const TEAM team, Field &field);
	// GUIで建築予定の場所を受け取る
	void receive_build_plan(Field &field);
	void give_solver_build_plan(void);
	// 職人の配列
	Array<Array<Craftsman>> craftsmen;
	// solverプログラム
	ChildProcess child{ U"solver.exe", Pipe::StdInOut };
	// 現在のターン、ポイント数のフォント
	const Font normal_font = Font(50, U"SourceHanSansJP-Medium.otf");
	// 建築物の数のフォント
	const Font small_font = Font(25, U"SourceHanSansJP-Medium.otf");
	// 職人の番号のフォント
	const Font craftsman_font = Font((int32)CELL_SIZE,  U"SourceHanSansJP-Medium.otf");
	// 試合のターン数
	int turn_num = 200;
	// 現在のターン数
	int turn_num_now = 0;
	// 現在のターン
	TEAM now_turn = TEAM::RED;
	// 持ち時間(ms)
	int time = 3000;
	// 建築予定
	Array<Array<bool>> is_build_plan;
};

void Game::operate_gui(Field &field){
	// ターン終了
	if(SimpleGUI::Button(U"ターン終了", { 700, 500 }) or KeyE.down()){
		field.calc_area();
		field.calc_point(TEAM::RED);
		field.calc_point(TEAM::BLUE);
		for(Array<Craftsman> &ary : craftsmen){
			for(Craftsman &craftsman : ary){
				craftsman.initialize();
			}
		}
		now_turn = not now_turn;
	}
}

void Game::operate_craftsman(const TEAM team, Field &field){
	int craftsman_num = 0;
	for(Craftsman& craftsman : craftsmen[team]){
		craftsman_num++;
		// 行動済みならcontinue
		if(craftsman.is_acted){
			continue;
		}
		// 動かす対象の職人を決める
		if(keyboard_craftsman[craftsman_num].down()){
			if(not craftsman.is_target){
				for(Craftsman& craftsman_tmp : craftsmen[team]){
					craftsman_tmp.is_target = false;
				}
			}
			craftsman.is_target ^= true;
		}

		// 動かす対象でなければcontinue
		if(not craftsman.is_target){
			continue;
		}

		// 押された矢印キーに対する操作方向
		Optional<Point> direction = get_pressed_pos();
		// 押されたz,x,cに対する操作モード
		Optional<ACT> mode = get_pressed_mode();

		if(direction != none){
			Line{ get_cell_center(craftsman.pos), get_cell_center(craftsman.pos + direction.value()) }.draw(LineStyle::RoundCap, 5, Palette::Orange);
		}

		if(direction != none and mode != none){
			bool acted = mode.has_value();

			switch(mode.value()){
				// 移動モード
			case ACT::MOVE:
				acted &= craftsman.move(field, *direction);
				break;
				// 建築モード
			case ACT::BUILD:
				acted &= craftsman.build(field, *direction);
				break;
				// 破壊モード
			case ACT::DESTROY:
				acted &= craftsman.destroy(field, *direction);
				break;
			}
			if(acted){
				craftsman.is_target = false;
				for(int i = 1; i < (int)craftsmen[team].size(); i++){
					if(!craftsmen[team][(craftsman_num + i - 1) % (int)craftsmen[team].size()].is_acted){
						craftsmen[team][(craftsman_num + i - 1) % (int)craftsmen[team].size()].is_target = true;
						break;
					}
				}
			}
			break;
		}
	}
}

void Game::display_field(void) const {
	for(const Array<Craftsman> &ary : craftsmen){
		int craftsman_num = 0;
		for(const Craftsman &craftsman : ary){
			if(craftsman.is_acted){
				get_grid_circle(craftsman.pos).draw(Palette::Darkgray);
			}else if(craftsman.is_target){
				get_grid_rect(craftsman.pos).drawFrame(2, 2, Palette::Yellow);
			}
			craftsman_font(craftsman_num++).drawAt(get_cell_center(craftsman.pos));
		}
	}
}

void Game::display_details(Field &field) const {
	if(now_turn == TEAM::RED){
		normal_font(U"赤チームの手番").draw(800, 550, Palette::Red);
	}else{
		normal_font(U"青チームの手番").draw(800, 550, Palette::Blue);
	}
	const Array<int> building_red = field.get_building(TEAM::RED);
	const Array<int> building_blue = field.get_building(TEAM::BLUE);
	normal_font(U"赤ポイント:{}"_fmt(field.get_point(TEAM::RED))).draw(800, 50, ((now_turn == TEAM::RED) ? Palette::Red : Palette::Black));
	small_font(U"城壁:{}  陣地:{}  城:{}"_fmt(building_red[0], building_red[1], building_red[2])).draw(850, 125, ((now_turn == TEAM::RED) ? Palette::Red : Palette::Black));
	normal_font(U"青ポイント:{}"_fmt(field.get_point(TEAM::BLUE))).draw(800, 200, ((now_turn == TEAM::BLUE) ? Palette::Blue: Palette::Black));
	small_font(U"城壁:{}  陣地:{}  城:{}"_fmt(building_blue[0], building_blue[1], building_blue[2])).draw(850, 275, ((now_turn == TEAM::BLUE) ? Palette::Blue: Palette::Black));
	const int point_diff = field.get_point(TEAM::RED) - field.get_point(TEAM::BLUE);
	normal_font(U"点差:{}"_fmt(point_diff)).draw(800, 350, (point_diff >= 0) ? ((point_diff == 0) ? Palette::Black : Palette::Red) : Palette::Blue);
	normal_font(U"ターン数:{}/{}"_fmt(turn_num_now + 1, turn_num)).draw(800, 450, Palette::Black);
}

void Game::give_solver_initialize(const bool is_first, Field &field){
	// フィールドの縦横
	child.ostream() << HEIGHT << std::endl << WIDTH << std::endl;
	// solver.exeを赤色とする
	child.ostream() << ((is_first) ? 0 : 1) << std::endl;
	// ターン数
	child.ostream() << turn_num << std::endl;
	// 持ち時間
	child.ostream() << time << std::endl;
	// 池の数と座標
	child.ostream() << field.get_ponds().size() << std::endl;
	for (const Point p : field.get_ponds()) {
		child.ostream() << p.y << std::endl << p.x << std::endl;
	}
	// 城の数と座標
	child.ostream() << field.get_castles().size() << std::endl;
	for (const Point p : field.get_castles()) {
		child.ostream() << p.y << std::endl << p.x << std::endl;
	}
	// REDのチームの職人
	child.ostream() << field.get_craftsmen(TEAM::RED).size() << std::endl;
	for(const Point p : field.get_craftsmen(TEAM::RED)){
		child.ostream() << p.y << std::endl << p.x << std::endl;
	}
	// BLUEチームの職人
	child.ostream() << field.get_craftsmen(TEAM::BLUE).size() << std::endl;
	for(const Point p : field.get_craftsmen(TEAM::BLUE)){
		child.ostream() << p.y << std::endl << p.x << std::endl;
	}
}

void Game::give_solver(const TEAM team){
	for(const Craftsman &craftsman : craftsmen[not team]){
		craftsman.output_act(child);
	}
}

void Game::receive_solver(const TEAM team, Field &field){
	for(Craftsman& craftsman : craftsmen[team]){
		craftsman.input_act(child, field);
	}
}

void Game::receive_build_plan(Field &field){
	for(int h = 0; h < HEIGHT; h++){
		for(int w = 0; w < WIDTH; w++){
			if(get_grid_rect({ w, h }).rightClicked()
				or get_grid_rect({w,h}).leftClicked()
				or (get_grid_rect({w,h}).mouseOver() and KeyLControl.down() )) {
				if(field.get_cell(h, w) & CELL::CASTLE){
					continue;
				}
				bool is_around_wall = true;
				for(auto& dydx : range_wall){
					if(not is_in_field(h + dydx.y, w + dydx.x)){
						continue;
					}
					if(not (field.get_cell(h + dydx.y, w + dydx.x) & CELL::POND)){
						is_around_wall = false;
						break;
					}
				}
				if(is_around_wall){
					continue;
				}
				is_build_plan[h][w] ^= true;
			}
		}
	}
}

void Game::give_solver_build_plan(void){
	int n = 0;
	for(const auto &ary : is_build_plan){
		for(const auto &cell : ary){
			if(cell){
				n++;
			}
		}
	}
	child.ostream() << n << std::endl;
	for(int h = 0; h < HEIGHT; h++){
		for(int w = 0; w < WIDTH; w++){
			if(is_build_plan[h][w]){
				child.ostream() << h << std::endl;
				child.ostream() << w << std::endl;
			}
		}
	}
}
