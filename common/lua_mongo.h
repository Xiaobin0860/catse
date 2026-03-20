#pragma once
#include <iostream>
using namespace std;

struct oid{
    char time[4];
    char mac[3];
    char pid[2];
    char inc[3];
};

extern oid g_oid;
void oid_init(oid* the, int id);
void oid_gen_next(oid* the);
void little_to_big(char *out, const char *p, const int len);
void big_to_little(char *out, const char *p);
#include<md5.h>

typedef enum {
	MONGOC_UPDATE_NONE = 0,
	MONGOC_UPDATE_UPSERT = 1 << 0,
	MONGOC_UPDATE_MULTI_UPDATE = 1 << 1,
} mongoc_update_flags_t;

struct lua_mongo{

    bson b, bb;
    mongo conn;
    mongo_cursor *cursor;
    static lua_State *pl;
    static char buf[1 << 23];
    static int buf_len;
    static int buf_len_client;
    static int buf_len_find;
    static int buf_len_next;

    void bson2table(const char *bson_data, lua_State *pl, bool array_flag){
        if(!lua_istable(pl, -1)){
            return;
        }

		int top_old = lua_gettop(pl);
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
		int top_new = lua_gettop(pl);
		if(top_old!=top_new) {
			luaL_error(pl, "mytest lua_mongo.bson2table top_old=%d,top_new=%d", top_old,top_new);		
		}
    }
    void init(){
        //bson_malloc_func = static_malloc;
        //bson_realloc_func = static_realloc;
        //bson_free_func = static_free;
        oid_init(&g_oid, 0);
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

    void register_lua(lua_State *pl){
        lua_mongo::pl = pl;
        luaL_Reg l[] = {
            {"auth", static_auth},
            {"client", static_client},
            {"id", static_id},
            {"index", static_index},
            {"insert", static_insert},
            {"find", static_find},
            {"md5", static_md5},
            {"next", static_next},
            {"remove", static_remove},
            {"update", static_update},
			{"count", static_count},
            {0, 0}
        };
        global::ref.reg_tb_func(pl, l, "lua_mongo");
    }
    void table2bson(lua_State *pl, bson *b, bool array_flag){
        if(!lua_istable(pl, -1)){
            return;
        }
		int top_old = lua_gettop(pl);
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
		int top_new = lua_gettop(pl);
		if(top_old!=top_new) {
			luaL_error(pl, "mytest lua_mongo.table2bson top_old=%d,top_new=%d", top_old,top_new);		
		}
    }
    void auth(lua_State *pl){
        if(mongo_cmd_authenticate(&conn, luaL_checkstring(pl, 1), luaL_checkstring(pl, 2), luaL_checkstring(pl, 3))){
            //luaL_error(pl, "err=%s lasterr=%s", conn.errstr, conn.lasterrstr);
        }
    }
    void client(lua_State *pl){
        if(mongo_client(&conn, luaL_checkstring(pl, 1), luaL_checkint(pl, 2))){
            luaL_error(pl, "err=%s lasterr=%s", conn.errstr, conn.lasterrstr);
        }
        buf_len_client = buf_len;
    }
    void find(lua_State *pl){
        mongo_cursor_destroy(cursor);
        cursor = 0;
        buf_len_find = 0;
        int limit = (int)lua_tonumber(pl, 4);
		int skip = (int)lua_tonumber(pl, 5);
        bson_destroy(&b);
        bson_destroy(&bb);
        reset_buf_len();
        bson_init(&b);
        bson_init(&bb);
        lua_settop(pl, 3);
        table2bson(pl, &b, false);
        lua_pop(pl, 1);
        table2bson(pl, &bb, false);
        bson_finish(&b);
        bson_finish(&bb);
		cursor = mongo_find(&conn, luaL_checkstring(pl, 1), &bb, &b, limit, skip, 0);
        if(!cursor){
            luaL_error(pl, "err=%s lasterr=%s", conn.errstr, conn.lasterrstr);
        }
        buf_len_find = buf_len;
    }

	void count(lua_State *pl)
	{
		bson_destroy(&b);
		reset_buf_len();
		bson_init(&b);
		lua_settop(pl, 3);
		table2bson(pl, &b, false);
		bson_finish(&b);
		double num = mongo_count(&conn, luaL_checkstring(pl, 1), luaL_checkstring(pl, 2), &b);
		lua_pushnumber(pl, (lua_Number)num);
	}

    int id(lua_State *pl){
        oid_gen_next(&g_oid);
        char tmp[24];
        little_to_big(tmp, (const char*)&g_oid, 12);
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
        reset_buf_len();
        bson_init(&b);

        //这里判断有没有uuid 如果有的话 那么就不再重新生成了.
        lua_getfield(pl, -1, "_id");
        size_t len = 0;
        const char *p = lua_tolstring(pl, -1 , &len);
        lua_pop(pl,1);
        if (len == 0) {
            lua_pushliteral(pl, "_id");
            oid_gen_next(&g_oid);
            char tmp[24];
            little_to_big(tmp, (const char*)&g_oid, 12);
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
        int buf_len_next_old = buf_len_next;
        buf_len_next = 0;
        reset_buf_len();
        if(mongo_cursor_next(cursor)){
            return 0;
        }
        if(buf_len < buf_len_next_old){
            buf_len_next = buf_len_next_old;
        }
        else{
            buf_len_next = buf_len;
        }
        bson2table(bson_data(mongo_cursor_bson(cursor)), pl, false);
        lua_pushboolean(pl, 1);
        return 1;
    }
    void remove(lua_State *pl){
        bson_destroy(&b);
        reset_buf_len();
        bson_init(&b);
        table2bson(pl, &b, false);
        bson_finish(&b);
        if(mongo_remove(&conn, luaL_checkstring(pl, 1), &b, 0)){
            luaL_error(pl, "err=%s lasterr=%s", conn.errstr, conn.lasterrstr);
        }
    }

    void update(lua_State *pl){
        int nflag = 0;
        int nArgNum = lua_gettop(pl);
        if(nArgNum == 4)
        {
			nflag = luaL_checkint(pl, 4);
            lua_pop(pl, 1);
        }
        bson_destroy(&b);
        bson_destroy(&bb);
        reset_buf_len();
        bson_init(&b);
        bson_init(&bb);
        if(!lua_istable(pl, -1))
        {
            luaL_error(pl, "update must be table1");
        }
        table2bson(pl, &b, false);
        lua_settop(pl, 2);
        if(!lua_istable(pl, -1))
        {
            luaL_error(pl, "update must be table2");
        }
        table2bson(pl, &bb, false);
        bson_finish(&b);
        bson_finish(&bb);
		if (mongo_update(&conn, luaL_checkstring(pl, 1), &bb, &b, nflag, 0)){
            luaL_error(pl, "err=%s lasterr=%s", conn.errstr, conn.lasterrstr);
        }
    }

    bool luaL_checkbool(lua_State *pL, int nArg)
    {
        bool result = false;
        if(lua_type(pL, nArg) == LUA_TBOOLEAN)
        {
            result = lua_toboolean(pL, nArg);
        }
        else
        {
            luaL_typerror(pL, nArg, lua_typename(pL, LUA_TBOOLEAN));
        }
        return result;
    }
    /*
       static void *static_malloc(size_t s){
//return malloc(s);

if(!s){
luaL_error(pl, "1static_malloc=%d", s);
}
if(sizeof(buf) < buf_len + s + sizeof(size_t)){
luaL_error(pl, "2static_malloc=%d", s);
}
     *(size_t*)(buf + buf_len) = s;
     buf_len += s + sizeof(size_t);
     return buf + buf_len - s;
     }
     static void *static_realloc(void *p, size_t s){
//return realloc(p, s);

if(!(buf <= p && p < buf + buf_len)){
luaL_error(pl, "1static_realloc=%d", s);
}
if(!s){
luaL_error(pl, "2static_realloc=%d", s);
}
if(sizeof(buf) < buf_len + s + sizeof(size_t)){
luaL_error(pl, "3static_realloc=%d", s);
}
size_t ss = *((size_t*)p - 1);
if(s <= ss){
luaL_error(pl, "4static_realloc=%d %d", s, ss);
}
if(buf + buf_len < ss + (char*)p){
luaL_error(pl, "5static_realloc=%d %d", s, ss);
}
     *(size_t*)(buf + buf_len) = s;
     memcpy(buf + buf_len + sizeof(size_t), p, ss);
     buf_len += s + sizeof(size_t);
     return buf + buf_len - s;
     }
     static void static_free(void *p){
//free(p);
}*/
static void reset_buf_len(){
    if(buf_len_find){
        if(buf_len_next){
            buf_len = buf_len_next;
        }
        else{
            buf_len = buf_len_find;
        }
    }
    else{
        buf_len = buf_len_client;
    }
}
static int static_auth(lua_State *pl){
    ref.auth(pl);
    return 0;
}
static int static_client(lua_State *pl){
    ref.client(pl);
    return 0;
}
static int static_id(lua_State *pl){
    return ref.id(pl);
}
static int static_index(lua_State *pl){
    ref.index(pl);
    return 0;
}
static int static_insert(lua_State *pl){
    ref.insert(pl);
    return 0;
}
static int static_find(lua_State *pl){
    ref.find(pl);
    return 0;
}
static int static_md5(lua_State *pl){
    ref.md5(pl);
    return 1;
}
static int static_remove(lua_State *pl){
    ref.remove(pl);
    return 0;
}
static int static_update(lua_State *pl){
    ref.update(pl);
    return 0;
}
static int static_count(lua_State *pl){
	ref.count(pl);
	return 1;
}
static int static_next(lua_State *pl){
    return ref.next(pl);
}
static lua_mongo ref;
};
