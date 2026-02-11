#pragma once

struct lua_config_reader{
	lua_State *pl;
	bool open(const char *filename){
		if(!pl){
			pl = luaL_newstate();
			if(luaL_dofile(pl, filename)){
				puts(lua_tostring(pl, -1));
				lua_pop(pl, 1);
				assert(0);
				return false;
			}
		}
		return true;
	}

	bool read_bool(const char *key) {
		char buf[512] = "return ";
		int buf_len = strlen(buf);
		int key_len = strlen(key);
		if(sizeof(buf) <= buf_len + key_len){
			assert(0);
			return false;
		}

		memcpy(buf + buf_len, key, key_len);
		buf[buf_len + key_len] = 0;
		int top_old = lua_gettop(pl);
		if(luaL_dostring(pl, buf)){
			puts(lua_tostring(pl, -1));
			lua_settop(pl, top_old);
			assert(0);
			return false;
		}

		bool ret = (bool)lua_toboolean(pl, -1);
		lua_settop(pl, top_old);
		return ret;
	}

	bool read_int(const char *key, int *val){
		char buf[512] = "return ";
		int buf_len = strlen(buf);
		int key_len = strlen(key);
		if(sizeof(buf) <= buf_len + key_len){
			return false;
		}
		memcpy(buf + buf_len, key, key_len);
		buf[buf_len + key_len] = 0;
		int top_old = lua_gettop(pl);
		if(luaL_dostring(pl, buf)){
			puts(lua_tostring(pl, -1));
			lua_settop(pl, top_old);
			assert(0);
			return false;
		}
		if(!lua_isnumber(pl, -1)){
			lua_settop(pl, top_old);
			assert(0);
			return false;
		}
		*val = (int)lua_tonumber(pl, -1);
		lua_settop(pl, top_old);
		return true;
	}
	bool read_string(const char *key, char *val, unsigned val_cap){
		char buf[512] = "return ";
		int buf_len = strlen(buf);
		int key_len = strlen(key);
		if(sizeof(buf) <= buf_len + key_len){
			return false;
		}
		memcpy(buf + buf_len, key, key_len);
		buf[buf_len + key_len] = 0;
		int top_old = lua_gettop(pl);
		if(luaL_dostring(pl, buf)){
			puts(lua_tostring(pl, -1));
			lua_settop(pl, top_old);
			assert(0);
			return false;
		}
		if(!lua_isstring(pl, -1)){
			lua_settop(pl, top_old);
			assert(0);
			return false;
		}
		size_t str_len = 0;
		const char *str = luaL_checklstring(pl, -1, &str_len);
		if(val_cap <= str_len){
			lua_settop(pl, top_old);
			assert(0);
			return false;
		}
		memcpy(val, str, str_len);
		val[str_len] = 0;
		lua_settop(pl, top_old);
		return true;
	}

	bool read_int_array(const char *key, short *val, short* val_cap)
	{
		char buf[512] = "return ";
		int buf_len = strlen(buf);
		int key_len = strlen(key);
		if(sizeof(buf) <= buf_len + key_len){
			return false;
		}
		memcpy(buf + buf_len, key, key_len);
		buf[buf_len + key_len] = 0;
		int top_old = lua_gettop(pl);
		if(luaL_dostring(pl, buf)){
			puts(lua_tostring(pl, -1));
			lua_settop(pl, top_old);
			assert(0);
			return false;
		}
		if(!lua_istable(pl, -1)){
			lua_settop(pl, top_old);
			assert(0);
			return false;
		}
		lua_rawgeti(pl, -1, 0);		
		size_t arr_len = luaL_checknumber(pl, -1);
		lua_pop(pl, 1);
		if(*val_cap < arr_len){
			lua_settop(pl, top_old);
			assert(0);
			return false;
		}
		for(int i = 1; i<=arr_len; i++)
		{
			lua_rawgeti(pl, -1, i);
			
			val[i-1] = luaL_checknumber(pl, -1);
			lua_pop(pl, 1);
		}
		*val_cap = arr_len;
		lua_settop(pl, top_old);
		return true;
	}
	static lua_config_reader ref;
};