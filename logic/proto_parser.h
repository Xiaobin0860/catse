#pragma once
struct proto_attr{
	char type;
	short cnt;
	short next;
	short cap;
	char name[64];
	proto_attr()
	{
		memset(this, 0, sizeof(proto_attr));
	}
};
struct proto_parser{
	enum{
		enum_byte,
		enum_short,
		enum_int,
		enum_uint,
		enum_double,
		enum_string,
	};

	proto_attr adj[32768][128];
	int next_node_id;
	
	char (*has_read_all)(void *);
	char (*get_last_char_read)(void *);
	char *(*get_last_char_write)(void *);
	char(*read_byte)(void *);
	short(*read_short)(void *);
	int(*read_int)(void*);
	void(*read_string)(void*, char *, const int);
	double(*read_double)(void*);
	void(*write_byte)(void*, char);
	void(*write_short)(void*, short);
	void(*write_double)(void*, double);
	void(*write_int)(void*, unsigned);
	void(*write_string)(void*, const char *, const int);
	
	
	char err_name[128];

	proto_parser(){
		next_node_id = 4096;
		memset(err_name, 0, sizeof(err_name));
		memset(adj, 0, sizeof(adj));
	}
	bool read_msg_dfs(void *data, int nodeid, lua_State *pl){
		int top_old = lua_gettop(pl);
		for(int i = 0; *adj[nodeid][i].name; ++i){
			const proto_attr &pa = adj[nodeid][i];
			if(pa.cnt < 2){
				if(pa.next){
					lua_getfield(pl, -1, pa.name);
					if (!read_msg_dfs(data, pa.next, pl)){
						assert(0);
						return false;
					}
					lua_pop(pl, 1);
				}else{
					if(pa.type == enum_byte){
						lua_pushnumber(pl, read_byte(data));
					}else if(pa.type == enum_short){
						lua_pushnumber(pl, read_short(data));
					}else if(pa.type == enum_int){
						lua_pushnumber(pl, read_int(data));
					}else if(pa.type == enum_uint){
						lua_pushnumber(pl, read_int(data));
					}else if(pa.type == enum_double){
						lua_pushnumber(pl, read_double(data));
					
					}else if(pa.type == enum_string){
						int len = read_short(data);
						if(len < 0){
							return false;
						}
						char buf[65536];
						if(sizeof(buf)<len){
							return false;
						}
						read_string(data, buf, len);
						lua_pushlstring(pl, buf, len);					
					}
					else{						
						assert(0);
						return false;
					}
					lua_setfield(pl, -2, pa.name);
				}
			}else{
				int len = read_short(data);
				if(!(0<=len && len<=pa.cnt)){
					printf("nodeid=%d, proto_id=%d, name=%s, len=%d, cap=%d\n", nodeid, ((msg_recv*)data)->id, pa.name, len, pa.cnt);
					return false;
				}
				lua_getfield(pl, -1, pa.name);
				if(!lua_istable(pl, -1)){
					luaL_error(pl, pa.name);
				}
				lua_pushnumber(pl, len);
				lua_rawseti(pl, -2, 0);
				for(int j = 0; j < len; ++j){
					if(pa.next){
						lua_rawgeti(pl, -1, j + 1);
						if(!lua_istable(pl, -1)){
							luaL_error(pl, "%s[%d]", pa.name, j + 1);
						}
						read_msg_dfs(data, pa.next, pl);
						lua_pop(pl, 1);
					}
					else{
						if(pa.type == enum_int){
							lua_pushnumber(pl, read_int(data));
						}
						else if(pa.type == enum_uint){
							lua_pushnumber(pl, read_int(data));
						}
						else if(pa.type == enum_short){
							lua_pushnumber(pl, read_short(data));
						}else if(pa.type == enum_byte){
							lua_pushnumber(pl, read_byte(data));
						}else if(pa.type == enum_double){
							lua_pushnumber(pl, read_double(data));
						}			
						else if(pa.type == enum_string){
							int len = read_short(data);
							if(len < 0){
								return false;
							}
							char buf[65536];
							if(sizeof(buf)<len){
								return false;
							}
							read_string(data, buf, len);
							lua_pushlstring(pl, buf, len);
						}
						else{
							assert(0);
							return false;
						}
						lua_rawseti(pl, -2, j + 1);
					}
				}
				lua_pop(pl, 1);
			}
		}
		int top_new = lua_gettop(pl);
		if(top_old!=top_new) {
			luaL_error(pl, "mytest proto_parser.read_msg_dfs top_old=%d,top_new=%d", top_old,top_new);		
		}
		return true;
	}

#define MY_CHECK_TYPE(t, ret) if(lua_type(pl, -1) != t) { strncpy(err_name, pa.name, 128); return ret*100+lua_type(pl, -1);}
	int write_msg_dfs(void *data, int nodeid, lua_State *pl){
		int top_old = lua_gettop(pl);
		if(!lua_istable(pl, -1))
		{
			return 15;
		}
		for(int i = 0; *adj[nodeid][i].name; ++i){
			const proto_attr &pa = adj[nodeid][i];
			lua_getfield(pl, -1, pa.name);
			if(pa.cnt < 2){
				if(pa.next){
					int ret = write_msg_dfs(data, pa.next, pl);
					if(ret) return ret;
				}else{
					if(pa.type == enum_byte){
						MY_CHECK_TYPE(LUA_TNUMBER, 1);
						write_byte(data, (char)luaL_checknumber(pl, -1));
					}else if(pa.type == enum_short){
						MY_CHECK_TYPE(LUA_TNUMBER, 2);
						write_short(data, (short)luaL_checknumber(pl, -1));
					}else if(pa.type == enum_double){				
						MY_CHECK_TYPE(LUA_TNUMBER, 3);
						write_double(data, luaL_checknumber(pl, -1));
					}else if(pa.type == enum_int){						
						MY_CHECK_TYPE(LUA_TNUMBER, 4);
						write_int(data, (unsigned)luaL_checknumber(pl, -1));
					}else if(pa.type == enum_uint){						
						MY_CHECK_TYPE(LUA_TNUMBER, 5);
						write_int(data, (unsigned)luaL_checknumber(pl, -1));
					}else if(pa.type == enum_string){						
						MY_CHECK_TYPE(LUA_TSTRING, 6);
						size_t len;
						const char *str = luaL_checklstring(pl, -1, &len);
						write_short(data, len);
						write_string(data, str, len);
					}else{						
						assert(0);
						return 6;
					}
				}
			}else{
				if(!lua_istable(pl, -1))
				{
					strncpy(err_name, pa.name, 128); 
					return 14;
				}
				lua_rawgeti(pl, -1, 0);
				MY_CHECK_TYPE(LUA_TNUMBER, 7);
				short len = (short)luaL_checknumber(pl, -1);
				lua_pop(pl, 1);
				write_short(data, len);
				for(int j = 0; j < len; ++j){
					lua_rawgeti(pl, -1, j + 1);
					if(pa.next){
						int ret = write_msg_dfs(data, pa.next, pl);
						if(ret) return ret;
					}else{
						if(pa.type == enum_byte){
							MY_CHECK_TYPE(LUA_TNUMBER, 8);
							write_byte(data, (char)luaL_checknumber(pl, -1));
						}else if(pa.type == enum_short){
							MY_CHECK_TYPE(LUA_TNUMBER, 9);
							write_short(data, (short)luaL_checknumber(pl, -1));
						
						}else if(pa.type == enum_double){
							MY_CHECK_TYPE(LUA_TNUMBER, 10);
							write_double(data, luaL_checknumber(pl, -1));
						}else if(pa.type == enum_int){
							MY_CHECK_TYPE(LUA_TNUMBER, 11);
							write_int(data, (unsigned)luaL_checknumber(pl, -1));
						}else if(pa.type == enum_uint){
							MY_CHECK_TYPE(LUA_TNUMBER, 11);
							write_int(data, (unsigned)luaL_checknumber(pl, -1));
						}else if(pa.type == enum_string){
							MY_CHECK_TYPE(LUA_TSTRING, 12);
							size_t len;
							const char *str = luaL_checklstring(pl, -1, &len);
							write_short(data, len);
							write_string(data, str, len);
						}else{
							assert(0);
							return 13;
						}
					}
					lua_pop(pl, 1);
				}
			}
			lua_pop(pl, 1);
		}
		int top_new = lua_gettop(pl);
		if(top_old!=top_new) {
			luaL_error(pl, "mytest proto_parser.write_msg_dfs top_old=%d,top_new=%d", top_old,top_new);		
		}

		return 0;
	}
	void template2tree_dfs(int nodeid, lua_State *pl){
		int top_old = lua_gettop(pl);
		if(sizeof(adj) / sizeof(*adj)<=nodeid){
			assert(0);
		}
		if(*adj[nodeid][0].name){
			assert(0);
			return;
		}
		for(int i = 0; ; ++i){
			lua_rawgeti(pl, -1, i + 1);
			if(!lua_istable(pl, -1)){
				lua_pop(pl, 1);
				break;
			}
			if(sizeof(*adj) / sizeof(**adj) <= i + 1){
				assert(0);
				exit(0);
			}
			lua_rawgeti(pl, -1, 1);
			size_t len;
			const char *p = luaL_checklstring(pl, -1, &len);
			if(len < 1){
				assert(0);
				exit(0);
			}
			if(sizeof(adj[nodeid][i].name) <= len){
				assert(0);
				exit(0);
			}
			memcpy(adj[nodeid][i].name, p, len + 1);
			lua_pop(pl, 1);

			lua_rawgeti(pl, -1, 2);
			adj[nodeid][i].cnt = (int)luaL_checknumber(pl, -1);
			lua_pop(pl, 1);

			lua_rawgeti(pl, -1, 3);
			if(lua_isstring(pl, -1)){
				adj[nodeid][i].next = 0;
				const char *type = luaL_checkstring(pl, -1);
				if(!strcmp(type, "byte")){
					adj[nodeid][i].type = enum_byte;
				}
				else if(!strcmp(type, "short")){
					adj[nodeid][i].type = enum_short;
				}
				else if(!strcmp(type, "double")){
					adj[nodeid][i].type = enum_double;
				}
				else if(!strcmp(type, "int")){
					adj[nodeid][i].type = enum_int;
				}
				else if(!strcmp(type, "uint")){
					adj[nodeid][i].type = enum_uint;
				}
				else if(!strcmp(type, "string")){
					adj[nodeid][i].type = enum_string;
					lua_rawgeti(pl, -2, 4);
					lua_pop(pl, 1);
				}
				else{
					printf("%d %s, not bit double int string\n", nodeid, adj[nodeid][i].name);
					assert(0);
				}
			}
			else if(lua_istable(pl, -1)){
				adj[nodeid][i].next = ++next_node_id;
				template2tree_dfs(adj[nodeid][i].next, pl);
			}
			else{
				printf("%d %s, not table\n", nodeid, adj[nodeid][i].name);
				assert(0);
			}
			lua_pop(pl, 1);

			lua_pop(pl, 1);
		}
		int top_new = lua_gettop(pl);
		if(top_old!=top_new) {
			luaL_error(pl, "mytest proto_parser.template2tree_dfs top_old=%d,top_new=%d", top_old,top_new);		
		}
	}
	
};