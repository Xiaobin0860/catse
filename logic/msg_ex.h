#pragma once

#include"proto_parser.h"


struct msg_ex{
	int log_gc_full;
	void *msg_read;
	const char *(*get_name)(int);

	proto_parser parser;

	void init();
	int read(lua_State *pl){
		int proto_id = (int)luaL_checknumber(pl, 1);
		if(parser.read_msg_dfs(msg_read, proto_id, pl) && parser.has_read_all(msg_read)){
			return 0;
		}
		else{
			lua_pushboolean(pl, 1);
			return 1;
		}
	}

	int register_template(lua_State *pl){
		int proto_id = (int)luaL_checknumber(pl, 1);
		parser.template2tree_dfs(proto_id, pl);
		return 0;
	}
	int broadcast(lua_State *pl);
	int multicast(lua_State *pl);
	int multicast(lua_State *pl, const short *fds, int fds_len);	
	int unicast(lua_State *pl);
	void register_lua(lua_State *pl);

	static int static_unicast(lua_State *pl);
	static int static_broadcast(lua_State *pl);
	static int static_multicast(lua_State *pl);
	static int static_read(lua_State *pl);
	static int static_register_template(lua_State *pl);
	

	static msg_ex ref;


};