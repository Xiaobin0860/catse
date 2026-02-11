#include "../logic/global.h"
#include<md5.h>

global global::ref;
struct lua_mongo_merge;

lua_mongo_merge * lua_mongo1;
lua_mongo_merge * lua_mongo2;
lua_mongo_merge * lua_mongo3;
lua_mongo_merge * lua_mongo4;
lua_mongo_merge * lua_mongo5;
lua_mongo_merge * lua_mongo6;
lua_mongo_merge * lua_mongo7;

struct lua_mongo_merge{
	short hash_little_to_big[256];
	char hash_big_to_little[256 * 256];
	bson b, bb;
	mongo conn;
	mongo_cursor *cursor;
	//lua_State *pl;
	void big_to_little(char *out, const char *p){
		for(int i = 0; i < 12; ++i){
			out[i] = hash_big_to_little[*(unsigned short*)(p + (i << 1))];
		}
	}
	void bson2table(const char *bson_data, lua_State *pl, bool array_flag){
		if(!lua_istable(pl, -1)){
			return;
		}

		bson_iterator i;
		bson_iterator_from_buffer(&i, bson_data);
		while(bson_iterator_next(&i)){
			int t = bson_iterator_type(&i);
			if (!t){
				break;
			}
			const char *key = bson_iterator_key(&i);
			if(is_unsigned_int(key)){
				lua_pushnumber(pl, atoi(key) + array_flag);
			}
			else{
				lua_pushstring(pl, key);
			}

			switch(t){
				case BSON_DOUBLE:
					lua_pushnumber(pl, bson_iterator_double(&i));
					break;
				case BSON_INT:
					lua_pushnumber(pl, bson_iterator_int(&i));
					break;
				case BSON_LONG:
					lua_pushnumber(pl, bson_iterator_long(&i));
					break;
				case BSON_STRING:
					lua_pushlstring(pl, bson_iterator_string(&i), bson_iterator_string_len(&i) - 1);
					break;
				case BSON_OBJECT:
					lua_pushvalue(pl, -1);
					lua_rawget(pl, -3);
					if(!lua_istable(pl, -1)){
						lua_pop(pl, 1);
						lua_newtable(pl);
					}
					bson2table(bson_iterator_value(&i), pl, false);
					break;
				case BSON_ARRAY:				
					lua_pushvalue(pl, -1);
					lua_rawget(pl, -3);
					if(!lua_istable(pl, -1)){
						lua_pop(pl, 1);
						lua_newtable(pl);
					}
					bson2table(bson_iterator_value(&i), pl, true);
					break;
				case BSON_BINDATA:
					lua_pushlstring(pl, bson_iterator_bin_data(&i), bson_iterator_bin_len(&i));
					break;
				case BSON_OID:
					char tmp[24];
					little_to_big(tmp, bson_iterator_value(&i), 12);
					lua_pushlstring(pl, tmp, sizeof(tmp)); 
					break;
				case BSON_BOOL:
					lua_pushboolean(pl, bson_iterator_bool(&i));
					break;
				default:
					luaL_error(pl, "bson2table t=%d", t);
			}
			lua_rawset(pl, -3);
		}
	}
	void init(){
		for(int i = 0; i < 256; ++i){
			sprintf((char*)(hash_little_to_big + i), "%02x", i);
			hash_big_to_little[(unsigned short)hash_little_to_big[i]] = i;
		}
	}
	const char* int2string(char *buf, int n){
		static char dp[8192][16];
		if(0<=n && n<sizeof(dp)/sizeof(*dp)){
			if(*dp[n]){
				return dp[n];
			}
			sprintf(dp[n], "%d", n);
			return dp[n];
		}
		sprintf(buf, "%d", n);
		return buf;
	}
	bool is_array(lua_State *pl){
		lua_pushnil(pl);
		int cnt = 0;
		while(lua_next(pl, -2)){
			int type_key = lua_type(pl, -2);
			if(type_key - LUA_TNUMBER){
				lua_pop(pl, 2);
				return false;
			}
			double d = lua_tonumber(pl, -2) - ++cnt;
			int z = 1e-8;
			if(!(-1e-8 < d && d < 1e-8)){
				lua_pop(pl, 2);
				return false;
			}
			lua_pop(pl, 1);
		}
		return true;
	}
	bool is_unsigned_int(const char *str){
		if(!*str){
			return false;
		}
		if(strlen(str) > 9) {
			return false;
		}
		for(; *str; ++str){
			if(!(47 < *str && *str < 58)){
				return false;
			}
		}
		return true;
	}
	void little_to_big(char *out, const char *p, const int len){
		for(int i = 0; i < len; ++i){
			*(short*)(out + (i << 1)) = hash_little_to_big[(unsigned char)p[i]];
		}
	}

	void table2bson(lua_State *pl, bson *b, bool array_flag){
		if(!lua_istable(pl, -1)){
			return;
		}
		lua_pushnil(pl);
		while(lua_next(pl, -2)){
			int type_key = lua_type(pl, -2);
			const char *key = 0;
			if(type_key == LUA_TNUMBER){
				char buf[16];
				key = int2string(buf, (int)lua_tonumber(pl, -2) - array_flag);
			}
			else if(type_key == LUA_TSTRING){
				key = lua_tostring(pl, -2);
			}
			else{
				luaL_error(pl, "table2bson type_key=%d", type_key);
			}

			if(strcmp(key, "_id")){
				int type_value = lua_type(pl, -1);
				switch(type_value){
					case LUA_TBOOLEAN:
						bson_append_bool(b, key, lua_toboolean(pl, -1));
						break;
					case LUA_TNUMBER:
						bson_append_double(b, key, lua_tonumber(pl, -1));
						break;
					case LUA_TSTRING:
						{
							size_t len = 0;
							const char *str = lua_tolstring(pl, -1, &len);
							int err_old = b->err;
							if(bson_append_string_n(b, key, str, len)){
								bson_append_binary(b, key, 0, str, len);
								b->err = err_old;
							}
							break;
						}
					case LUA_TTABLE:
						if(is_array(pl)){
							bson_append_start_array(b, key);
							table2bson(pl, b, true);
							bson_append_finish_array(b);
						}
						else{
							bson_append_start_object(b, key);
							table2bson(pl, b, false);
							bson_append_finish_object(b);
						}
						break;
					default:
						luaL_error(pl, "table2bson value_type=%d", type_value);
				}
			}
			else{
				int type_value = lua_type(pl, -1);
				if(type_value == LUA_TSTRING){
					size_t len = 0;
					const char *val = lua_tolstring(pl, -1, &len);
					char tmp[12] = {};
					if(len == 24){
						big_to_little(tmp, val);
					}
					bson_append_oid(b, "_id", (bson_oid_t*)tmp);
				}
				else{
					luaL_error(pl, "table2bson _id not string");
				}
			}
			lua_pop(pl, 1);
		}
	}
	void auth(lua_State *pl){
		if(mongo_cmd_authenticate(&conn, luaL_checkstring(pl, 1), luaL_checkstring(pl, 2), luaL_checkstring(pl, 3))){
			if (conn.errcode != 0 || conn.lasterrcode != 0) luaL_error(pl, "err=%s lasterr=%s", conn.errstr, conn.lasterrstr);
		}
	}
	void client(lua_State *pl){
		if(mongo_client(&conn, luaL_checkstring(pl, 1), 27017)){
			luaL_error(pl, "err=%s lasterr=%s", conn.errstr, conn.lasterrstr);
		}
	}
	void find(lua_State *pl){
		mongo_cursor_destroy(cursor);
		cursor = 0;
		int limit = (int)lua_tonumber(pl, 4);
		bson_destroy(&b);
		bson_destroy(&bb);
		bson_init(&b);
		bson_init(&bb);
		lua_settop(pl, 3);
		table2bson(pl, &b, false);
		lua_pop(pl, 1);
		table2bson(pl, &bb, false);
		bson_finish(&b);
		bson_finish(&bb);
		cursor = mongo_find(&conn, luaL_checkstring(pl, 1), &bb, &b, limit, 0, 0);
		if(!cursor){
			luaL_error(pl, "err=%s lasterr=%s", conn.errstr, conn.lasterrstr);
		}
	}
	int id(lua_State *pl){
		bson_oid_t oid;
		bson_oid_gen(&oid);
		char tmp[24];
		little_to_big(tmp, (const char*)&oid, 12);
		lua_pushlstring(pl, tmp, sizeof(tmp));
		return 1;
	}
	void index(lua_State *pl){
		if(mongo_create_simple_index(&conn, luaL_checkstring(pl, 1), luaL_checkstring(pl, 2), 0, 0)){
			luaL_error(pl, "err=%s lasterr=%s", conn.errstr, conn.lasterrstr);
		}
	}
	void insert(lua_State *pl){
		if(!lua_istable(pl, -1)){
			luaL_error(pl, "insert !lua_istable(pl, -1)");
		}
		bson_destroy(&b);
		bson_init(&b);

		//这里判断有没有uuid 如果有的话 那么就不再重新生成了
		lua_getfield(pl, -1, "_id");
		size_t len = 0;
		const char *p = lua_tolstring(pl, -1 , &len);
		lua_pop(pl,1);
		if (len == 0) {
			lua_pushliteral(pl, "_id");
			bson_oid_t oid;
			bson_oid_gen(&oid);
			char tmp[24];
			little_to_big(tmp, (const char*)&oid, 12);
			lua_pushlstring(pl, tmp, sizeof(tmp));
			lua_rawset(pl, -3);		
		}

		table2bson(pl, &b, false);
		bson_finish(&b);
		if(mongo_insert(&conn, luaL_checkstring(pl, 1), &b, 0)){
			luaL_error(pl, "err=%s lasterr=%s", conn.errstr, conn.lasterrstr);
		}
	}
	void md5(lua_State *pl){
		size_t len;
		const char *p = luaL_checklstring(pl, 1, &len);
		mongo_md5_state_t st;
		mongo_md5_init(&st);
		mongo_md5_append(&st, (unsigned char*)p, len);
		char aa[16];
		mongo_md5_finish(&st, (unsigned char*)aa);
		char bb[32];
		little_to_big(bb, aa, sizeof(aa));
		lua_pushlstring(pl, bb, sizeof(bb));
	}
	int next(lua_State *pl){
		if(!cursor){
			luaL_error(pl, "next !cursor");
		}
		if(mongo_cursor_next(cursor)){
			return 0;
		}
		
		bson2table(bson_data(mongo_cursor_bson(cursor)), pl, false);
		lua_pushboolean(pl, 1);
		return 1;
	}
	void remove(lua_State *pl){
		bson_destroy(&b);
		bson_init(&b);
		table2bson(pl, &b, false);
		bson_finish(&b);
		if(mongo_remove(&conn, luaL_checkstring(pl, 1), &b, 0)){
			luaL_error(pl, "err=%s lasterr=%s", conn.errstr, conn.lasterrstr);
		}
	}
	void update(lua_State *pl){
		bson_destroy(&b);
		bson_destroy(&bb);
		bson_init(&b);
		bson_init(&bb);
		table2bson(pl, &b, false);
		lua_settop(pl, 2);
		table2bson(pl, &bb, false);
		bson_finish(&b);
		bson_finish(&bb);
		if(mongo_update(&conn, luaL_checkstring(pl, 1), &bb, &b, 0, 0)){
			luaL_error(pl, "err=%s lasterr=%s", conn.errstr, conn.lasterrstr);
		}
	}

	void register_lua1(lua_State *pl){
		luaL_Reg l[] = {
			{"index", static_index1},
			{"auth", static_auth1},
			{"client", static_client1},
			{"insert", static_insert1},
			{"find", static_find1},
			{"next", static_next1},
			{"remove", static_remove1},
			{"update", static_update1},
			{0, 0}
		};
		global::ref.reg_tb_func(pl, l, "lua_mongo1");
	}
	
	static int static_index1(lua_State *pl){
		lua_mongo1->index(pl);
		return 0;
	}

	static int static_auth1(lua_State *pl){
		lua_mongo1->auth(pl);
		return 0;
	}

	static int static_client1(lua_State *pl){
		lua_mongo1->client(pl);
		return 0;
	}
	static int static_insert1(lua_State *pl){
		lua_mongo1->insert(pl);
		return 0;
	}
	static int static_find1(lua_State *pl){
		lua_mongo1->find(pl);
		return 0;
	}
	static int static_remove1(lua_State *pl){
		lua_mongo1->remove(pl);
		return 0;
	}
	static int static_update1(lua_State *pl){
		lua_mongo1->update(pl);
		return 0;
	}
	static int static_next1(lua_State *pl){
		return lua_mongo1->next(pl);
	}

	void register_lua2(lua_State *pl){
		luaL_Reg l[] = {
			{"index", static_index2},
			{"auth", static_auth2},
			{"client", static_client2},
			{"insert", static_insert2},
			{"find", static_find2},
			{"next", static_next2},
			{"remove", static_remove2},
			{"update", static_update2},
			{0, 0}
		};
		global::ref.reg_tb_func(pl, l, "lua_mongo2");
	}
		
	static int static_index2(lua_State *pl){
		lua_mongo2->index(pl);
		return 0;
	}

	static int static_auth2(lua_State *pl){
		lua_mongo2->auth(pl);
		return 0;
	}

	static int static_client2(lua_State *pl){
		lua_mongo2->client(pl);
		return 0;
	}
	static int static_insert2(lua_State *pl){
		lua_mongo2->insert(pl);
		return 0;
	}
	static int static_find2(lua_State *pl){
		lua_mongo2->find(pl);
		return 0;
	}
	static int static_remove2(lua_State *pl){
		lua_mongo2->remove(pl);
		return 0;
	}
	static int static_update2(lua_State *pl){
		lua_mongo2->update(pl);
		return 0;
	}
	static int static_next2(lua_State *pl){
		return lua_mongo2->next(pl);
	}

	void register_lua3(lua_State *pl){
		luaL_Reg l[] = {
			{"index", static_index3},
			{"auth", static_auth3},
			{"client", static_client3},
			{"insert", static_insert3},
			{"find", static_find3},
			{"next", static_next3},
			{"remove", static_remove3},
			{"update", static_update3},
			{0, 0}
		};
		global::ref.reg_tb_func(pl, l, "lua_mongo3");
	}
		
	static int static_index3(lua_State *pl){
		lua_mongo3->index(pl);
		return 0;
	}

	static int static_auth3(lua_State *pl){
		lua_mongo3->auth(pl);
		return 0;
	}

	static int static_client3(lua_State *pl){
		lua_mongo3->client(pl);
		return 0;
	}
	static int static_insert3(lua_State *pl){
		lua_mongo3->insert(pl);
		return 0;
	}
	static int static_find3(lua_State *pl){
		lua_mongo3->find(pl);
		return 0;
	}
	static int static_remove3(lua_State *pl){
		lua_mongo3->remove(pl);
		return 0;
	}
	static int static_update3(lua_State *pl){
		lua_mongo3->update(pl);
		return 0;
	}
	static int static_next3(lua_State *pl){
		return lua_mongo3->next(pl);
	}

	void register_lua4(lua_State *pl){
		luaL_Reg l[] = {
			{"index", static_index4},
			{"auth", static_auth4},
			{"client", static_client4},
			{"insert", static_insert4},
			{"find", static_find4},
			{"next", static_next4},
			{"remove", static_remove4},
			{"update", static_update4},
			{0, 0}
		};
		global::ref.reg_tb_func(pl, l, "lua_mongo4");
	}

	static int static_index4(lua_State *pl){
		lua_mongo4->index(pl);
		return 0;
	}

	static int static_auth4(lua_State *pl){
		lua_mongo4->auth(pl);
		return 0;
	}

	static int static_client4(lua_State *pl){
		lua_mongo4->client(pl);
		return 0;
	}
	static int static_insert4(lua_State *pl){
		lua_mongo4->insert(pl);
		return 0;
	}
	static int static_find4(lua_State *pl){
		lua_mongo4->find(pl);
		return 0;
	}
	static int static_remove4(lua_State *pl){
		lua_mongo4->remove(pl);
		return 0;
	}
	static int static_update4(lua_State *pl){
		lua_mongo4->update(pl);
		return 0;
	}
	static int static_next4(lua_State *pl){
		return lua_mongo4->next(pl);
	}

	void register_lua5(lua_State *pl){
		luaL_Reg l[] = {
			{"index", static_index5},
			{"auth", static_auth5},
			{"client", static_client5},
			{"insert", static_insert5},
			{"find", static_find5},
			{"next", static_next5},
			{"remove", static_remove5},
			{"update", static_update5},
			{0, 0}
		};
		global::ref.reg_tb_func(pl, l, "lua_mongo5");
	}

	static int static_index5(lua_State *pl){
		lua_mongo5->index(pl);
		return 0;
	}

	static int static_auth5(lua_State *pl){
		lua_mongo5->auth(pl);
		return 0;
	}

	static int static_client5(lua_State *pl){
		lua_mongo5->client(pl);
		return 0;
	}
	static int static_insert5(lua_State *pl){
		lua_mongo5->insert(pl);
		return 0;
	}
	static int static_find5(lua_State *pl){
		lua_mongo5->find(pl);
		return 0;
	}
	static int static_remove5(lua_State *pl){
		lua_mongo5->remove(pl);
		return 0;
	}
	static int static_update5(lua_State *pl){
		lua_mongo5->update(pl);
		return 0;
	}
	static int static_next5(lua_State *pl){
		return lua_mongo5->next(pl);
	}

	void register_lua6(lua_State *pl){
		luaL_Reg l[] = {
			{ "index", static_index6 },
			{ "auth", static_auth6 },
			{ "client", static_client6 },
			{ "insert", static_insert6 },
			{ "find", static_find6 },
			{ "next", static_next6 },
			{ "remove", static_remove6 },
			{ "update", static_update6 },
			{ 0, 0 }
		};
		global::ref.reg_tb_func(pl, l, "lua_mongo6");
	}

	static int static_index6(lua_State *pl){
		lua_mongo6->index(pl);
		return 0;
	}

	static int static_auth6(lua_State *pl){
		lua_mongo6->auth(pl);
		return 0;
	}

	static int static_client6(lua_State *pl){
		lua_mongo6->client(pl);
		return 0;
	}
	static int static_insert6(lua_State *pl){
		lua_mongo6->insert(pl);
		return 0;
	}
	static int static_find6(lua_State *pl){
		lua_mongo6->find(pl);
		return 0;
	}
	static int static_remove6(lua_State *pl){
		lua_mongo6->remove(pl);
		return 0;
	}
	static int static_update6(lua_State *pl){
		lua_mongo6->update(pl);
		return 0;
	}
	static int static_next6(lua_State *pl){
		return lua_mongo6->next(pl);
	}

	void register_lua7(lua_State *pl){
		luaL_Reg l[] = {
			{ "index", static_index7 },
			{ "auth", static_auth7 },
			{ "client", static_client7 },
			{ "insert", static_insert7 },
			{ "find", static_find7 },
			{ "next", static_next7 },
			{ "remove", static_remove7 },
			{ "update", static_update7 },
			{ 0, 0 }
		};
		global::ref.reg_tb_func(pl, l, "lua_mongo7");
	}

	static int static_index7(lua_State *pl){
		lua_mongo7->index(pl);
		return 0;
	}

	static int static_auth7(lua_State *pl){
		lua_mongo7->auth(pl);
		return 0;
	}

	static int static_client7(lua_State *pl){
		lua_mongo7->client(pl);
		return 0;
	}
	static int static_insert7(lua_State *pl){
		lua_mongo7->insert(pl);
		return 0;
	}
	static int static_find7(lua_State *pl){
		lua_mongo7->find(pl);
		return 0;
	}
	static int static_remove7(lua_State *pl){
		lua_mongo7->remove(pl);
		return 0;
	}
	static int static_update7(lua_State *pl){
		lua_mongo7->update(pl);
		return 0;
	}
	static int static_next7(lua_State *pl){
		return lua_mongo7->next(pl);
	}
};

int main(int argc, char **argv){
	mongo_env_sock_init();
	
	lua_State *pl = luaL_newstate();
	luaL_openlibs(pl);
	lua_pushcfunction(pl, global::static_traceback);
	int errfunc = lua_gettop(pl);

	lua_mongo1 = new lua_mongo_merge();
	lua_mongo1->init();
	lua_mongo1->register_lua1(pl);

	lua_mongo2 = new lua_mongo_merge();
	lua_mongo2->init();
	lua_mongo2->register_lua2(pl);

	lua_mongo3 = new lua_mongo_merge();
	lua_mongo3->init();
	lua_mongo3->register_lua3(pl);

	lua_mongo4 = new lua_mongo_merge();
	lua_mongo4->init();
	lua_mongo4->register_lua4(pl);

	lua_mongo5 = new lua_mongo_merge();
	lua_mongo5->init();
	lua_mongo5->register_lua5(pl);

	lua_mongo6 = new lua_mongo_merge();
	lua_mongo6->init();
	lua_mongo6->register_lua6(pl);

	lua_mongo7 = new lua_mongo_merge();
	lua_mongo7->init();
	lua_mongo7->register_lua7(pl);

	if(luaL_loadfile(pl, "script/merge.lua")){
		puts(lua_tostring(pl, -1));
		lua_pop(pl, 1);
		assert(0);
		exit(0);
		return 1;
	}
	
	if(lua_pcall(pl, 0, 0, errfunc)){
		puts(lua_tostring(pl, -1));
		lua_pop(pl, 1);
		assert(0);
		exit(0);
		return 1;
	}

	return 0;
}
