# pragma once
# include <Siv3D.hpp> // OpenSiv3D v0.6.10
# include "Game.hpp"


// 人対solver
class PvC : public App::Scene, public Game {
public:
	PvC(const InitData &init);
	void operate_gui(Field &field);
	void display_field(void) const;
	void update() override;
	void draw() const override;
private:
	TEAM team_solver = TEAM::RED;
};

PvC::PvC(const InitData &init) : IScene(init){
	craftsmen.resize(2);
	is_build_plan.resize(HEIGHT, Array<bool>(WIDTH, false));
	for(int h = 0; h < HEIGHT; h++){
		for(int w = 0; w < WIDTH; w++){
			if(getData().get_cell(h, w) & CELL::CRAFTSMAN_RED){
				craftsmen[TEAM::RED].push_back(Craftsman(h, w, TEAM::RED));
			}else if(getData().get_cell(h, w) & CELL::CRAFTSMAN_BLUE){
				craftsmen[TEAM::BLUE].push_back(Craftsman(h, w, TEAM::BLUE));
			}
		}
	}
	give_solver_initialize(team_solver == TEAM::RED, getData());

	// Computerが先手の場合
	if(team_solver == TEAM::RED){
		System::Sleep(1.5s);
		now_turn = (not now_turn);
		receive_solver(team_solver, getData());
		for(Array<Craftsman>& ary : craftsmen){
			for(Craftsman& craftsman : ary){
				craftsman.initialize();
			}
		}
		getData().calc_area();
		getData().calc_point(TEAM::RED);
		getData().calc_point(TEAM::BLUE);
	}
}

void PvC::operate_gui(Field &field){
	// ターン終了
	if(SimpleGUI::Button(U"ターン終了", { 700, 500 }) or KeyE.down()){
		field.calc_area();
		field.calc_point(TEAM::RED);
		field.calc_point(TEAM::BLUE);
		give_solver(team_solver);
		//give_solver_build_plan();
		System::Sleep(1.5s);
		now_turn = not now_turn;
		receive_solver(team_solver, field);
		for(Array<Craftsman>& ary : craftsmen){
			for(Craftsman& craftsman : ary){
				craftsman.initialize();
			}
		}
		now_turn = not now_turn;
		field.calc_area();
		field.calc_point(TEAM::RED);
		field.calc_point(TEAM::BLUE);
	}
}

void PvC::display_field(void) const {
	for(const Array<Craftsman> &ary : craftsmen){
		int craftsman_num = 1;
		for(const Craftsman &craftsman : ary){
			if(craftsman.is_acted){
				get_grid_circle(craftsman.pos).draw(Palette::Darkgray);
			}else if(craftsman.is_target){
				get_grid_rect(craftsman.pos).drawFrame(2, 2, Palette::Yellow);
			}
			craftsman_font(craftsman_num++).drawAt(get_cell_center(craftsman.pos), Palette::Black);
		}
	}
	for(int h = 0; h < HEIGHT; h++){
		for(int w = 0; w < WIDTH; w++){
			if(is_build_plan[h][w]){
				get_grid_rect({ h,w }).drawFrame(1, 1, Palette::Darkviolet);
			}
		}
	}
}

void PvC::update(){
	operate_gui(getData());
	operate_craftsman(now_turn, getData());
	receive_build_plan(getData());
}

void PvC::draw() const {
	getData().display_grid();
	getData().display_actors();
	display_field();
	display_details(getData());
}
