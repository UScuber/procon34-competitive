# pragma once
# include <Siv3D.hpp>

class MenuRect {
public:
	MenuRect(Point coordinate, Size size, ColorF colorf, String label, Font& font);
	// クリックされたか
	bool is_clicked(void);
	// 描画
	void draw_menu_rect(void);
private:
	// 四角形の左上のyx座標
	Point coordinate;
	// 四角形の大きさ
	Size size;
	// 四角形のフォント
	ColorF colorf;
	// 表示する文字
	String label;
	// 四角形
	Rect rect;
	// フォント
	Font& font;
};

MenuRect::MenuRect(Point coordinate, Size size, ColorF colorf, String label, Font& font) :font(font) {
	this->coordinate = coordinate;
	this->size = size;
	this->colorf = colorf;
	this->label = label;
	this->rect = Rect{ this->coordinate, this->size };
}

bool MenuRect::is_clicked(void) {
	return this->rect.leftClicked();
}

// 四角形と文字
void MenuRect::draw_menu_rect(void) {
	this->rect.draw(this->colorf);
	this->font(this->label).drawAt(this->coordinate + this->size / 2, Palette::Black);
}

// ウィンドウ上部のメニュータブ
class MenuTab {
public:
	MenuTab(size_t number, size_t height, Font& font);
	// 描画
	void draw_menu_tab(void);
	// どのMenuRectがクリックされたか(返り値は0-idx)
	Optional<size_t> what_clicked(void);
	MenuTab& operator=(const MenuTab& menutab) {
		this->number = menutab.number;
		this->height = menutab.height;
		this->font = menutab.font;
		this->menurects = menutab.menurects;
	}
private:
	// メニュータブの個数
	size_t number = 1;
	// 幅はウィンドウサイズの幅で固定
	size_t height;
	// フォント
	Font& font;
	// 四角形配列
	Array<MenuRect> menurects;
};

MenuTab::MenuTab(size_t number, size_t height, Font& font) :font(font) {
	assert(number > 0);
	this->number = number;
	this->height = height;
	for (size_t i = 0; i < number; i++) {
		this->menurects << MenuRect(Point{ 1280 / this->number * i, 0 }, Size{ 1280 / this->number, this->height }, ColorF{ 0.5, 0.5 }, U"TEST", this->font);
	}
}

void MenuTab::draw_menu_tab(void) {
	for (MenuRect& rect : this->menurects) {
		rect.draw_menu_rect();
	}
}

Optional<size_t> MenuTab::what_clicked(void) {
	for (size_t i = 0; i < menurects.size(); i++) {
		if (menurects[i].is_clicked()) {
			return i;
		}
	}
	return none;
}
