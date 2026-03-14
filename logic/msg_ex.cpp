#include "global.h"
#include "msg_ex.h"
#include "thread_log.h"
#include "thread_logic.h"
#include "thread_io.h"
#include "pk_limit.h"



msg_ex msg_ex::ref;

void msg_ex::init()
{
	parser.get_last_char_read = msg_recv::static_get_last_char_read;
	parser.get_last_char_write = msg_send::static_get_last_char_write;
	parser.read_byte = msg_recv::static_read_byte;
	parser.read_int = msg_recv::static_read_int;
	parser.read_short = msg_recv::static_read_short;
	parser.read_double = msg_recv::static_read_double;
	parser.read_string = msg_recv::static_read_string;
	parser.write_byte = msg_send::static_write_byte;
	parser.write_short = msg_send::static_write_short;
	parser.write_double = msg_send::static_write_double;
	parser.write_int = msg_send::static_write_int;
	parser.write_string = msg_send::static_write_string;	
	parser.has_read_all = msg_recv::static_has_read_all;

	get_name = pk_limit::static_get_name;
}
int msg_ex::static_broadcast(lua_State *pl){
	return ref.broadcast(pl);
}
int msg_ex:: static_multicast(lua_State *pl){
	return ref.multicast(pl);
}
int msg_ex:: static_read(lua_State *pl){
	return ref.read(pl);
}
int msg_ex:: static_register_template(lua_State *pl){
	return ref.register_template(pl);
}
int msg_ex:: static_unicast(lua_State *pl){
	return ref.unicast(pl);
}


void msg_ex::register_lua(lua_State *pl){
	luaL_Reg l[] = {
		{"unicast", static_unicast},
		{"broadcast", static_broadcast},
		{"multicast", static_multicast},
		{"read", static_read},		
		{"register_template", static_register_template},
		{0, 0}
	};
	global::ref.reg_tb_func(pl, l, "msg_ex");
}

int msg_ex::broadcast(lua_State *pl){
	if(!lua_istable(pl, 1)){
		luaL_error(pl, "lua_istable");
	}
	lua_rawgeti(pl, 1, 1);
	int proto_id = (int)luaL_checknumber(pl, -1);
	lua_pop(pl, 1);
	msg_send *msg = global::ref.msg_send_pool.alloc();
	if(!msg){
		char buf[512];
		sprintf(buf, "broadcast,%d,%s", proto_id, get_name(proto_id));
		thread_log::ref.write_c_log_logic(pl, log_gc_full, buf);
		assert(0);
		return 0;
	}
	msg->buf_len = 0;
	msg->id = proto_id;	
	int ret = parser.write_msg_dfs(msg, proto_id, pl);
	if(ret)
	{
		//失败，需要释放msg
		global::ref.msg_send_pool.free(msg);
		luaL_error(pl, "broadcast:protoid=%d, ret=%d\n", proto_id, ret);
	}

	msg->fds_len = -1;
	thread_io::ref.q_send.push(msg);
	return 0;
}
int msg_ex::multicast(lua_State *pl){
	if(!lua_istable(pl, 1)){
		luaL_error(pl, "!lua_istable");
	}
	lua_rawgeti(pl, 1, 1);
	int proto_id = (int)luaL_checknumber(pl, -1);
	lua_pop(pl, 1);
	if(!lua_istable(pl, 2)){
		luaL_error(pl, "!lua_istable");
	}
	lua_rawgeti(pl, 2, 0);
	int fds_len = (int)luaL_checknumber(pl, -1);
	if(!fds_len){
		return 0;
	}
	lua_pop(pl, 1);
	msg_send *msg = global::ref.msg_send_pool.alloc();
	if(!msg){
		char buf[512];
		sprintf(buf, "multicast,%d,%s", proto_id, get_name(proto_id));
		thread_log::ref.write_c_log_logic(pl, log_gc_full, buf);
		assert(0);
		return 0;
	}
	msg->fds_len = 0;
	
	for(int i = 1; i <= fds_len; ++i){
		lua_rawgeti(pl, 2, i);
		if(lua_type(pl, -1) != LUA_TNUMBER)
		{
			global::ref.msg_send_pool.free(msg);
			luaL_error(pl, "multicast error:%d", lua_type(pl, -1));
		}
		msg->push_fd((int)luaL_checknumber(pl, -1));
		lua_pop(pl, 1);
	}
	lua_pop(pl, 1);
	msg->buf_len = 0;
	msg->id = proto_id;

	int ret = parser.write_msg_dfs(msg, proto_id, pl);
	if(ret)
	{
		//失败，需要释放msg
		global::ref.msg_send_pool.free(msg);
		luaL_error(pl, "multicast:protoid=%d, ret=%d\n", proto_id, ret);
	}
	thread_io::ref.q_send.push(msg);
	return 0;
}


int msg_ex::unicast(lua_State *pl){
	if(lua_isnil(pl, 2)){
		return 0;
	}
	short fd = (short)luaL_checknumber(pl, 2);
	lua_pop(pl, 1);

	if(!lua_istable(pl, 1)){
		luaL_error(pl, "lua_istable");
	}
	lua_rawgeti(pl, 1, 1);
	int proto_id = (int)luaL_checknumber(pl, -1);
	lua_pop(pl, 1);
	msg_send *msg = global::ref.msg_send_pool.alloc();
	if(!msg){
		char buf[512];
		sprintf(buf, "unicast,%d,%s", proto_id, get_name(proto_id));
		thread_log::ref.write_c_log_logic(pl, log_gc_full, buf);
		assert(0);
		return 0;
	}
	msg->buf_len = 0;
	msg->id = proto_id;;
	int ret = parser.write_msg_dfs(msg, proto_id, pl);
	if(ret)
	{
		//失败，需要释放msg
		global::ref.msg_send_pool.free(msg);
		luaL_error(pl, "unicast:protoid=%d ->%s ret=%d\n", proto_id, parser.err_name, ret);
	}
	msg->fds_len = 0;
	msg->push_fd(fd);

	thread_io::ref.q_send.push(msg);

	return 0;
}