# pragma once
# include <Siv3D.hpp> // OpenSiv3D v0.6.10
# include "Game.hpp"


class Start : public App::Scene {
public:
	Start(const InitData &init) : IScene(init){};
	void update() override {
		if(SimpleGUI::Button(U"Start", { 100, 100 })){
			const URL url = U"localhost:5000/start";
			const HashTable<String, String> headers{ { U"Content-Type", U"application/json" } };
			SimpleHTTP::Get(url, headers, U"./tmp");
			changeScene(U"CvC", 0s);
		}
	}
};
