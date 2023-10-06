# pragma once
# include <Siv3D.hpp> // OpenSiv3D v0.6.10
# include "Game.hpp"


// 人対人
class PvP : public App::Scene, public Game {
public:
	PvP(const InitData &init);
	void update() override;
	void draw() const override;
};

// コンストラクタで職人配列をセット
PvP::PvP(const InitData &init) : IScene(init){
	craftsmen.resize(2);
	for(int h = 0; h < HEIGHT; h++){
		for(int w = 0; w < WIDTH; w++){
			if(getData().get_cell(h, w) & CELL::CRAFTSMAN_RED){
				craftsmen[TEAM::RED].push_back(Craftsman(h, w, TEAM::RED));
			}else if(getData().get_cell(h, w) & CELL::CRAFTSMAN_BLUE){
				craftsmen[TEAM::BLUE].push_back(Craftsman(h, w, TEAM::BLUE));
			}
		}
	}
}

void PvP::update(){
	operate_gui(getData());
	operate_craftsman(now_turn, getData());
}

void PvP::draw() const {
	getData().display_grid();
	getData().display_actors();
	display_details(getData());
}
