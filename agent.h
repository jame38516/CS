#pragma once
#include <string>
#include <random>
#include <sstream>
#include <map>
#include <type_traits>
#include <algorithm>
#include "board.h"
#include "action.h"
#include "weight.h"
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

//////////////////////////////////////////////////////////////////////
/**
 * base agent for agents with weight tables
 */
class weight_agent : public agent {
public:
	weight_agent(const std::string& args = "") : agent(args) {
		if (meta.find("init") != meta.end()) // pass init=... to initialize the weight
			init_weights(meta["init"]);
		if (meta.find("load") != meta.end()) // pass load=... to load from a specific file
			load_weights(meta["load"]);
	}
	virtual ~weight_agent() {
		if (meta.find("save") != meta.end()) // pass save=... to save to a specific file
			save_weights(meta["save"]);
	}

protected:
	virtual void init_weights(const std::string& info) {
		for (int i=0; i<8; i++)
			net.emplace_back(50625); // create an empty weight table with size 65536
		//net.emplace_back(65536); // create an empty weight table with size 65536
		// now net.size() == 2; net[0].size() == 65536; net[1].size() == 65536
	}
	virtual void load_weights(const std::string& path) {
		std::ifstream in(path, std::ios::in | std::ios::binary);
		if (!in.is_open()) std::exit(-1);
		uint32_t size;
		in.read(reinterpret_cast<char*>(&size), sizeof(size));
		net.resize(size);
		for (weight& w : net) in >> w;
		in.close();
	}
	virtual void save_weights(const std::string& path) {
		std::ofstream out(path, std::ios::out | std::ios::binary | std::ios::trunc);
		if (!out.is_open()) std::exit(-1);
		uint32_t size = net.size();
		out.write(reinterpret_cast<char*>(&size), sizeof(size));
		for (weight& w : net) out << w;
		out.close();
	}
	friend class player;
protected:
	std::vector<weight> net;
};

/**
 * base agent for agents with a learning rate
 */
class learning_agent : public agent {
public:
	learning_agent(const std::string& args = "") : agent(args), alpha(0.1/32) {
		if (meta.find("alpha") != meta.end())
			alpha = float(meta["alpha"]);
	}
	virtual ~learning_agent() {}

protected:
	float alpha;
};
///////////////////////////////////////////////////



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
//weight_agent ag("init=0");	
//class player : public random_agent {
class player : public weight_agent {
public:
	std::vector<vector<int>> s_all;
	std::vector<int> reward_all;
	vector<int> val, val_tem;
	player(const std::string& args = "") : weight_agent(args) {}
	 
	virtual action take_action(const board& before) {
		//std::shuffle(opcode.begin(), opcode.end(), engine);
		board::reward reward = 0;
		board::reward max_reward = reward;
		bool action_flag = false; 
		
	//	learning_agent ag_l();
		flag = true;
		board tmp;
		int value, max_value;
		for (int op=0; op<=3; op++) {		
		//	board tem = before;	
			tmp = before;
			reward = tmp.slide(op);
			
			if (reward != -1) {
				value = 0;	
/*				
				for (int i=0; i<=15; i++){
					cout<< tmp(i);
					if ((i+1)%4 == 0)
						cout << "\n";
				}*/
//				cout << reward << "\n";
				for (int i=0; i<4; i++){
					for (int j=0; j<4; j++)
						s.push_back(tmp(j + 4*i));
					value += net[i][3375*s[0] + 225*s[1] + 15*s[2] + s[3]];
//					cout << value << " ";
					vector<int>().swap(s);
				}
				
				for (int i=4; i<8; i++){
					for (int j=0; j<4; j++)
						s.push_back(tmp(4*j + (i-4)));
					value += net[i][3375*s[0] + 225*s[1] + 15*s[2] + s[3]];
//					cout << value << " ";
					vector<int>().swap(s);
				}
//				cout << "\n";
				reward += value;	
				action_flag = true;
				if (reward >= max_reward){
					max_reward = reward;
					max_value = value;
					action_flag = true;
					mv = op;
				}
			}
		}
		
//		cout << "mv:" << mv << "\n";
		 
		
	//	cout << "max_reward:" << max_reward << "\n";
		
		
		if (action_flag){
			reward_all.push_back(max_reward-max_value);
			tmp = before;
			tmp.slide(mv);
			for (int i=0; i<4; i++){
				for (int j=0; j<4; j++)
					s.push_back(tmp(j + 4*i));
				s_all.push_back(s);
				vector<int>().swap(s);
			}
					
			for (int i=0; i<4; i++){
				for (int j=0; j<4; j++)
					s.push_back(tmp(4*j + i));
				s_all.push_back(s);
				vector<int>().swap(s);
			}
			return action::slide(mv);
		}
		
		else{
		//	reward_all.push_back(max_reward-value);
/*			for (int i=0; i<50625; i++){
				cout << ag.net[0][i] << " ";
			}
			cout << "\n";
			*/
	//		cout << "reward:"<< reward_all.size() << " "<< s_all.size()/8 << "\n";

			for (int j=0; j<s_all.size()/8; j++){
				
				int pre_value = 0, value = 0;
				for (int i=0; i<8; i++){
				//	for (int k=0; k<4; k++){
					int index = s_all.size() - 8*(j+1) + i;
					int weight_index = 3375*s_all[index][0] + 225*s_all[index][1] + 15*s_all[index][2] + s_all[index][3];
					if (j == 0)
						net[i][weight_index] = 0;
					else{
						int prev_index = s_all.size() - 8*j + i;
						int prev_weight_index = 3375*s_all[prev_index][0] + 225*s_all[prev_index][1] + 15*s_all[prev_index][2] + s_all[prev_index][3];
						pre_value += net[i][prev_weight_index];
						value += net[i][weight_index];
					}
						
				//	}
					
				}
				if (j != 0)
					for (int i=0; i<8; i++){
						int index = s_all.size() - 8*(j+1) + i;
						int weight_index = 3375*s_all[index][0] + 225*s_all[index][1] + 15*s_all[index][2] + s_all[index][3];
						net[i][weight_index] += 0.1/32*(pre_value - value + reward_all[reward_all.size()-j]);
					}
							
						
					
//					cout << "value:" << ag.net[i][weight_index] << "\n";

			}	
			vector<vector<int>>().swap(s_all);
			vector<int>().swap(reward_all);
//			cout << "game end-------------------\n\n";
			return action();
		}
			
	}

private:
	std::array<int, 4> opcode;
	std::vector<int> s;
	vector<int> rew;
	bool flag = false;
	int count = 0;
	//weight_agent ag;
};








