# pragma once
# include <Siv3D.hpp> // OpenSiv3D v0.6.10
# include "CvC.hpp"
# include "PvP.hpp"
# include "PvC.hpp"
# include "StartScene.hpp"


// +--------------------+
//  |  solver.exe : 赤  |
//  |  server     : 青     |
// +--------------------+


void Main(){
	// ウィンドウ設定
	Scene::SetBackground(Palette::Lightgreen);
	Window::Resize(1280, 720);

	App manager;
	manager.add<PvP>(U"PvP");
	manager.add<PvC>(U"PvC");
	manager.add<CvC>(U"CvC");
	manager.add<Start>(U"Start");

	manager.init(U"Start");
	
	while(System::Update()){
		if(not manager.update()){
			break;
		}
	}

}
