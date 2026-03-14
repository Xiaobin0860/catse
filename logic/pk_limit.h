#pragma once

struct pk_limit
{
	enum{
		err_ok,
		err_fd_invalid,
		err_id_invalid,
		err_fixing,
		err_too_fast
	};
	static const int fd_max = 16384;
	static const int id_max = 3072; //以CG_开头的包最大ID
	char cnt[id_max];   //一个间隔之内最多为多少次.
	short second[id_max];//多少毫秒为一个间隔.
	char last_cnt[fd_max][id_max];    //记录该fd该协议号次数.
	long long last_time[fd_max][id_max];   //记录该fd该协议号上一次的时间.
	char name[id_max][64];
	int check_pk_limit(long long cur_time, int fd, int id){
		if (!(0<=fd&&fd<fd_max)){
			return err_fd_invalid;
		}
		if (!(0<=id&&id<id_max)){
			return err_id_invalid;
		}
		if (cnt[id] <= 0){
			return err_fixing;
		}
		if (second[id] <= 0){
			return err_ok;
		}
		if (cur_time < last_time[fd][id] + second[id] && cnt[id] <= last_cnt[fd][id]){
			//last_time[fd][id] = cur_time;
			return err_too_fast;
		}
		if (last_time[fd][id] + second[id] <= cur_time){
			last_time[fd][id] = cur_time;
			last_cnt[fd][id] = 0;
		}
		++last_cnt[fd][id];
		return err_ok;
	}
	const char *get_name(int id){
		return 0<=id&&id<id_max ? name[id] : "nil";
	}
	void register_lua(lua_State *pl){
		luaL_Reg l[] = {
			{"set_limit", static_set_limit},
			{"set_name", static_set_name},
			{0, 0}
		};
		global::ref.reg_tb_func(pl, l, "pk_limit");
	}
	int set_limit(lua_State *pl){
		int key = (short)luaL_checknumber(pl, 1);
		if(!(0<=key && key<id_max)){
			luaL_error(pl, "key = %d", key);
		}
		second[key] = (short)luaL_checknumber(pl, 2);
		cnt[key] = (char)luaL_checknumber(pl, 3);
		return 0;
	}
	int set_name(lua_State *pl){
		int key = (short)luaL_checknumber(pl, 1);
		if(!(0<=key && key<id_max)){
			luaL_error(pl, "key = %d", key);
		}
		size_t len = 0;
		const char *p = luaL_checklstring(pl, 2, &len);
		if(sizeof(name[key])<=len){
			luaL_error(pl, "%s too long", p);
		}
		memcpy(name[key], p, len + 1);
		return 0;
	}
	static const char *static_get_name(int key){
		return ref.get_name(key);
	}
	static int static_set_limit(lua_State *pl){
		return ref.set_limit(pl);
	}
	static int static_set_name(lua_State *pl){
		return ref.set_name(pl);
	}
	static pk_limit ref;
};