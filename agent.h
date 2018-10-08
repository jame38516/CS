#pragma once
#include <string>
#include <random>
#include <sstream>
#include <map>
#include <type_traits>
#include <algorithm>
#include "board.h"
#include "action.h"
int mv = -1;
std::vector<int> tile_array;
class agent {
public:
	agent(const std::string& args = "") {
		std::stringstream ss("name=unknown role=unknown " + args);
		for (std::string pair; ss >> pair; ) {
			std::string key = pair.substr(0, pair.find('='));
			std::string value = pair.substr(pair.find('=') + 1);
			meta[key] = { value };
		}
	}
	virtual ~agent() {}
	virtual void open_episode(const std::string& flag = "") {}
	virtual void close_episode(const std::string& flag = "") {}
	virtual action take_action(const board& b) { return action(); }
	virtual bool check_for_win(const board& b) { return false; }

public:
	virtual std::string property(const std::string& key) const { return meta.at(key); }
	virtual void notify(const std::string& msg) { meta[msg.substr(0, msg.find('='))] = { msg.substr(msg.find('=') + 1) }; }
	virtual std::string name() const { return property("name"); }
	virtual std::string role() const { return property("role"); }

protected:
	typedef std::string key;
	struct value {
		std::string value;
		operator std::string() const { return value; }
		template<typename numeric, typename = typename std::enable_if<std::is_arithmetic<numeric>::value, numeric>::type>
		operator numeric() const { return numeric(std::stod(value)); }
	};
	std::map<key, value> meta;
};

class random_agent : public agent {
public:
	random_agent(const std::string& args = "") : agent(args) {
		if (meta.find("seed") != meta.end())
			engine.seed(int(meta["seed"]));
	}
	virtual ~random_agent() {}

protected:
	std::default_random_engine engine;
};

/**
 * random environment
 * add a new random tile to an empty cell
 * 2-tile: 90%
 * 4-tile: 10%
 */
class rndenv : public random_agent {
public:
	rndenv(const std::string& args = "") : random_agent("name=random role=environment " + args),
		space({ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 }) {}
	virtual action take_action(const board& after) {
	//	cout << mv << "\n";
		space.clear();
		if (mv == -1)
			space.assign({ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 });
		else if (mv == 0)
			space.assign({ 12, 13, 14, 15 });
		else if (mv == 1)
			space.assign({ 0, 4, 8, 12 });
		else if (mv == 2)
			space.assign({ 0, 1, 2, 3});
		else
			space.assign({3, 7, 11, 15});
		
		std::shuffle(space.begin(), space.end(), engine);
	/*	
		for (int i=0; i<=15; i++){
			cout<< after(i);
			if ((i+1)%4 == 0)
				cout << "\n"; 
		}
		cout << "\n"; 
	*/
		for (int pos : space) {
			if (after(pos) != 0) continue;
			//board::cell tile = popup(engine) ? 1 : 2;
			if ( tile_array.empty() ){
				
				tile_array = {1, 2, 3};
				std::shuffle(tile_array.begin(), tile_array.end(), engine);
				//tile_array[tile_array.end()-1];
				tile = tile_array.back();
				tile_array.pop_back();
			}
			else{
				tile = tile_array.back();
				tile_array.pop_back();
			}
			
			return action::place(pos, tile);
		}
		return action();
	}

private:
	std::vector<int> space;
	
	std::uniform_int_distribution<int> popup;
	board::cell tile;
};

/**
 * dummy player
 * select a legal action randomly
 */

class player : public random_agent {
public:
	player(const std::string& args = "") : random_agent("name=dummy role=player " + args),
		opcode({ 0, 1, 2, 3 }) {}

	virtual action take_action(const board& before) {
		std::shuffle(opcode.begin(), opcode.end(), engine);
		for (int op : opcode) {
			
			board::reward reward = board(before).slide(op);
			if (reward != -1) {
				mv = op;
				return action::slide(op);
			}
			
		}
		return action();
	}

private:
	std::array<int, 4> opcode;
};
