# pragma once
# include <Siv3D.hpp>

// フィールドの縦横
int HEIGHT;
int WIDTH;
// 一つのセルの大きさ(正方形)
constexpr int CELL_SIZE = 25;
// フィールドの左上に開ける余白
constexpr int BLANK_LEFT = 50;
constexpr int BLANK_TOP = 50;


struct CELL {
	static const CELL NONE;
	static const CELL POND;
	static const CELL WALL_RED;
	static const CELL WALL_BLUE;
	static const CELL AREA_RED;
	static const CELL AREA_BLUE;
	static const CELL CRAFTSMAN_RED; // 敵のagent
	static const CELL CRAFTSMAN_BLUE; // 味方のagent
	static const CELL CASTLE;
	static const CELL CRAFTSMAN; // CRAFTSMAN_BLUE | Enemey
	static const CELL WALL; // WALL_BLUE | WALL_RED
	static const CELL AREA; // AREA_BLUE | AREA_RED

	inline constexpr CELL(const unsigned char v=(unsigned char)0) : val(v){}
	inline constexpr CELL &operator|=(const CELL s) noexcept { val |= s.val; return *this; }
	inline constexpr CELL &operator&=(const CELL s) noexcept { val &= s.val; return *this; }
	inline constexpr CELL &operator^=(const CELL s) noexcept { val ^= s.val; return *this; }
	inline constexpr CELL operator|(const CELL s) const noexcept { return CELL(val | s.val); }
	inline constexpr CELL operator&(const CELL s) const noexcept { return CELL(val & s.val); }
	inline constexpr CELL operator^(const CELL s) const noexcept { return CELL(val ^ s.val); }
	inline constexpr CELL operator~() const noexcept { return CELL(~val); }
	inline constexpr operator bool() const noexcept { return val; }
	inline constexpr bool operator==(const CELL s) const noexcept { return val == s.val; }
	friend std::ostream &operator<<(std::ostream &os, const CELL s){ return os << (int)s.val; }

protected:
	unsigned char val;
};
constexpr CELL CELL::NONE = 0;
constexpr CELL CELL::POND = 1 << 0;
constexpr CELL CELL::WALL_RED = 1 << 1;
constexpr CELL CELL::WALL_BLUE = 1 << 2;
constexpr CELL CELL::AREA_RED = 1 << 3;
constexpr CELL CELL::AREA_BLUE = 1 << 4;
constexpr CELL CELL::CRAFTSMAN_RED = 1 << 5;
constexpr CELL CELL::CRAFTSMAN_BLUE = 1 << 6;
constexpr CELL CELL::CASTLE = 1 << 7;
constexpr CELL CELL::CRAFTSMAN = CELL::CRAFTSMAN_BLUE | CELL::CRAFTSMAN_RED;
constexpr CELL CELL::WALL = CELL::WALL_BLUE | CELL::WALL_RED;
constexpr CELL CELL::AREA = CELL::AREA_BLUE | CELL::AREA_RED;

struct TEAM {
	static const TEAM RED;
	static const TEAM BLUE;
	inline constexpr TEAM(const unsigned char v = (unsigned char)0) : val(v){}
	inline constexpr TEAM operator!() const noexcept { return TEAM(!val); }
	inline constexpr bool operator==(const TEAM t) const noexcept { return val == t.val; }
	inline constexpr operator bool() const noexcept { return val; }
protected:
	unsigned char val;
};
constexpr TEAM TEAM::RED = 0;
constexpr TEAM TEAM::BLUE = 1;

enum class ACT {
	NOTHING = 0,
	MOVE = 1,
	BUILD = 2,
	DESTROY = 3
};


bool is_in_field(const int y, const int x){
	return 0 <= y and y < HEIGHT and 0 <= x and x < WIDTH;
}
bool is_in_field(const Point p){
	return is_in_field(p.y, p.x);
}

CELL switch_cell(const CELL type, const TEAM team){
	if(type == CELL::WALL){
		return (team == TEAM::RED) ? CELL::WALL_RED : CELL::WALL_BLUE;
	}else if(type == CELL::AREA){
		return (team == TEAM::RED) ? CELL::AREA_RED : CELL::AREA_BLUE;
	}else if(type == CELL::CRAFTSMAN){
		return (team == TEAM::RED) ? CELL::CRAFTSMAN_RED : CELL::CRAFTSMAN_BLUE;
	}else{
		return CELL::NONE;
	}
}
