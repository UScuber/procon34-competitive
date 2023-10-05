# pragma once
# include <Siv3D.hpp>
# include "Base.hpp"


//　↖　↑　↗　→　↘　↓　↙　←　に変更
const Array<int> direction_client = { 4, 0, 7, 3, 6, 2, 5, 1 };
// 　↑　←　↓　→　↖　↙　↘　↗　に変更
const Array<int> direction_server = { 1, 7, 5, 3, 0, 6, 4, 2 };

// クライアントのdirectionの番号からサーバーの番号に変更
int to_direction_server(const int direction_num_client){
	return direction_server[direction_num_client];
}
// サーバーのdirectionの番号からクライアントの番号に変更
int to_direction_client(const int direction_num_server){
	return direction_client[direction_num_server];
}

Array<Array<int>> get_2d_array(const JSON& json){
	Array<Array<int>> res;
	for(const auto &object : json.arrayView()){
		Array<int> ary;
		for(const auto &num : object.arrayView()){
			ary << num.get<int>();
		}
		res << ary;
	}
	return res;
}


struct MatchDataBoard {
	int width = 0;
	int height = 0;
	int mason = 0;
	Array<Array<int>> structures = Array<Array<int>>(0);
	Array<Array<int>> masons = Array<Array<int>>(0);

	MatchDataBoard(){}

	MatchDataBoard(const JSON &board){
		this->width = board[U"width"].get<int>();
		this->height = board[U"height"].get<int>();
		this->mason = board[U"mason"].get<int>();
		this->structures = get_2d_array(board[U"structures"]);
		this->masons = get_2d_array(board[U"masons"]);
	}
};

struct MatchDataMatch {
	int id = 0;
	int turns = 0;
	int turnSeconds = 0;
	MatchDataBoard board = MatchDataBoard();
	String opponent = U"";
	bool first = false;

	MatchDataMatch(){}

	MatchDataMatch(const JSON &matches){
		this->id = matches[U"id"].get<int>();
		this->turns = matches[U"turns"].get<int>();
		this->turnSeconds = matches[U"turnSeconds"].get<int>();
		this->board = MatchDataBoard(matches[U"board"]);
		this->opponent = matches[U"opponent"].get<String>();
		this->first = matches[U"first"].get<bool>();
	}
};

struct MatchData {
	Array<MatchDataMatch> matches = Array<MatchDataMatch>(0);

	MatchData(){}

	MatchData(const JSON &json){
		for(const MatchDataMatch &matchdatamatch : json[U"matches"].arrayView()){
			matches << matchdatamatch;
		}
	}
};

struct MatchStatusBoard {
	int width = 0;
	int height = 0;
	int mason = 0;
	Array<Array<int>> walls = Array<Array<int>>(0);
	Array<Array<int>> territories = Array<Array<int>>(0);
	Array<Array<int>> structures = Array<Array<int>>(0);
	Array<Array<int>> masons = Array<Array<int>>(0);

	MatchStatusBoard(){}

	MatchStatusBoard(const JSON &board){
		this->width = board[U"width"].get<int>();
		this->height = board[U"height"].get<int>();
		this->mason = board[U"mason"].get<int>();
		this->walls = get_2d_array(board[U"walls"]);
		this->territories = get_2d_array(board[U"territories"]);
		this->structures = get_2d_array(board[U"structures"]);
		this->masons = get_2d_array(board[U"masons"]);
	}
};

struct MatchStatusLogAction {
	bool succeeded = false;
	int type = 0;
	int dir = 0;

	MatchStatusLogAction(){}

	MatchStatusLogAction(const JSON &log_action){
		this->succeeded = log_action[U"succeeded"].get<bool>();
		this->type = log_action[U"type"].get<int>();
		this->dir = log_action[U"dir"].get<int>();
	}
};

struct MatchStatusLog {
	int turn = 0;
	Array<MatchStatusLogAction> actions;

	MatchStatusLog(){}

	MatchStatusLog(const JSON &log){
		this->turn = log[U"turn"].get<int>();
		for(const JSON &matchstatuslogaction : log[U"actions"].arrayView()){
			this->actions << MatchStatusLogAction(matchstatuslogaction);
		}
	}
};

struct MatchStatus {
	int id = 0;
	int turn = 0;
	MatchStatusBoard board = MatchStatusBoard();
	Array<MatchStatusLog> logs = Array<MatchStatusLog>();

	MatchStatus(){}

	MatchStatus(const JSON &json){
		this->id = json[U"id"].get<int>();
		this->turn = json[U"turn"].get<int>();
		this->board = MatchStatusBoard(json[U"board"]);
		for(const MatchStatusLog &matchstatuslog : json[U"logs"].arrayView()){
			logs << matchstatuslog;
		}
	}
};

struct ActionPlanAction {
	int type = 0;
	int dir = 0;

	ActionPlanAction(){}

	ActionPlanAction(const int type, const int dir){
		this->type = type;
		this->dir = dir;
	}

	JSON output_json(void) const {
		JSON json;
		json[U"type"] = this->type;
		json[U"dir"] = this->dir;
		if(this->type == (int)ACT::NOTHING){
			json[U"dir"] = 0;
		}
		return json;
	}
};
struct  ActionPlan {
	int turn = 0;
	Array<ActionPlanAction> actions = Array<ActionPlanAction>(0);

	ActionPlan(){}

	ActionPlan(const int turn){
		this->turn = turn;
	}

	void push_back_action(const int& type, const int& dir){
		actions << ActionPlanAction(type, dir);
	}

	JSON output_json(void) const {
		JSON json;
		json[U"turn"] = this->turn;
		for(const ActionPlanAction &actionplanaction : actions){
			json[U"actions"].push_back(actionplanaction.output_json());
		}
		return json;
	}
};


class Connect {
private:
	const URL url_base = U"http://localhost:3000/";
	const HashTable<String, String> headers{ { U"Content-Type", U"application/json" } };
	int match_id = 0;
	String token = U"";

public:
	Connect(void);
	// 試合一覧取得
	Optional<MatchDataMatch> get_matches_list(void);
	// 試合状況取得
	Optional<MatchStatus> get_match_status(void);
	// 行動計画更新
	Optional<int> post_action_plan(const ActionPlan &action);
};


void output_console_response(const HTTPResponse &response){
	Console << U"------";
	Console << U"status code: " << FromEnum(response.getStatusCode());
}

void output_console_json(const JSON &json){
	Console << U"------";
	Console << json.format();
}

void output_console_fail(const String &str){
	Console << U"------";
	Console << str << U" failed!!";
	Console << U"------";
}

Connect::Connect(void){
	// solver.exeのチームトークンをローカルファイルから取得
	TextReader reader_token{ U"./token.env" };
	if(not reader_token){
		throw Error{ U"Failed to open 'token.env'" };
	}
	reader_token.readLine(token);
	// 行う試合のidをローカルファイルから取得
	TextReader reader_id{ U"./id.env" };
	if(not reader_id){
		throw Error{ U"Failed to open 'id.env'" };
	}
	String tmp_id = U"";
	reader_id.readLine(tmp_id);
	match_id = Parse<int>(tmp_id);
}

Optional<MatchDataMatch> Connect::get_matches_list(void){
	const URL url = url_base + U"matches" + U"?token=" + token;
	const FilePath saveFilePath = U"./mathes_list.json";
	if(const auto response = SimpleHTTP::Get(url, headers, saveFilePath)){
		output_console_response(response);
		if(response.isOK()){
			output_console_json(JSON::Load(saveFilePath));
			const MatchData matchdata = MatchData(JSON::Load(saveFilePath));
			for(const MatchDataMatch &matchdatamatch : matchdata.matches){
				if(matchdatamatch.id == this->match_id){
					return matchdatamatch;
				}
			}
			output_console_fail(U"get same match id");
		}
	}else{
		output_console_fail(U"get_matches_list");
	}
	return none;
}

Optional<MatchStatus> Connect::get_match_status(void){
	const URL url = url_base + U"matches/" + Format(match_id) + U"?token=" + token;
	const FilePath saveFilePath = U"./match_status.json";
	if(const auto response = SimpleHTTP::Get(url, headers, saveFilePath)){
		//output_console_response(response);
		if(response.isOK()){
			//output_console_json(JSON::Load(saveFilePath));
			return MatchStatus(JSON::Load(saveFilePath));
		}
	}else{
		output_console_fail(U"get_match_status");
	}
	return none;
}

Optional<int> Connect::post_action_plan(const ActionPlan &actionplan){
	const URL url = url_base + U"matches/" + Format(match_id) + U"?token=" + token;
	const FilePath saveFilePath = U"./tmp.json";
	const std::string data = actionplan.output_json().formatUTF8();
	if(const auto response = SimpleHTTP::Post(url, headers, data.data(), data.size(), saveFilePath)){
		output_console_response(response);
		if(response.isOK()){
			output_console_json(JSON::Load(saveFilePath));
			return (JSON::Load(saveFilePath))[U"accepted_at"].get<int>();
		}else{
			output_console_fail(U"post_action_plan");
		}
	}else{
		Print << U"Failed";
	}
	return none;
}
