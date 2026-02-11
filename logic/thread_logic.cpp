#include "global.h"
#include "lua_mongo.h"
#include "thread_log.h"
#include "thread_logic.h"
#include "thread_io.h"
#include "pk_limit.h"
#include "thread_hot.h"
#include "thread_http.h"
#include "thread_loop.h"
#include "msg_ex.h"
#include "msg_inner_parse.h"


thread_logic thread_logic::ref;
pk_limit pk_limit::ref;
thread_loop thread_loop::ref;

tm threadLogLocalTime;

void thread_logic::init()
{
	
	hot_time = 0;

	time_t t = time(0);
	threadLogLocalTime = *localtime(&t);

	q_recv_io.init(global::ref.msg_recv_pool.cap + 1, false);

	q_disconnect.init(global::ref.msg_recv_pool.cap + 1, false);

	q_recv_admin.init(global::ref.admin_recv_pool.cap + 1, false);
	q_send_admin.init(global::ref.admin_send_pool.cap + 1, false);


	tid = 0;


	log_msg = thread_log::ref.reg("log/err_msg_logic", "", 0, false);

	log_time = thread_log::ref.reg("log/err_time_logic", "", 0, false);

	log_hot = thread_log::ref.reg("log/err_hot_logic", "", 0, false);

	log_admin = thread_log::ref.reg("log/err_admin", "", 0, false);

	

	msg_ex::ref.log_gc_full = thread_log::ref.reg("log/err_gc_full", "function, id, name", 0, false);




	pl = luaL_newstate();
	luaL_openlibs(pl);
	lua_pushcfunction(pl, global::static_traceback);
	errfunc = lua_gettop(pl);
	global::ref.register_lua(pl);
	lua_mongo::ref.init();
	lua_mongo::ref.register_lua(pl);

	msg_inner_parse::ref.register_lua(pl);

	pk_limit::ref.register_lua(pl);

	msg_ex::ref.init();
	msg_ex::ref.register_lua(pl);

	thread_loop::ref.init(pl, false);

	thread_log::ref.register_lua(pl);

	thread_http::ref.register_lua(pl);



	//lua_register(pl, "server_info", server_info);
	lua_register(pl, "get_thread_id",     static_get_thread_id);

	if(luaL_loadfile(pl, global::ref.logic_main_file_name)){
		puts(lua_tostring(pl, -1));
		lua_pop(pl, 1);
		assert(0);
		exit(0);
	}
	if(lua_pcall(pl, 0, 0, errfunc)){
		puts(lua_tostring(pl, -1));
		lua_pop(pl, 1);
		assert(0);
		exit(0);
	}

}

void thread_logic::run()
{
	time_t t;
	tid = global::ref.get_thread_id();
	for(;;)
	{
		sleep1;
		t = time(0);
		thread_loop::ref.run_times++;
		threadLogLocalTime = *localtime(&t);
		int top_old = lua_gettop(pl);		
		handle_io();
		int top_new = lua_gettop(pl);
		if(top_old!=top_new) {
			thread_log::ref.write_c_log_err(pl, log_msg, "mytest after handle_io,%d,%d",top_old,top_new);	
			top_old = top_new;
		}
		handle_inner();
		top_new = lua_gettop(pl);
		if(top_old!=top_new) {
			thread_log::ref.write_c_log_err(pl, log_msg, "mytest after handle_inner,%d,%d",top_old,top_new);	
			top_old = top_new;
		}
		handle_admin();
		top_new = lua_gettop(pl);
		if(top_old!=top_new) {
			thread_log::ref.write_c_log_err(pl, log_msg, "mytest after handle_admin,%d,%d",top_old,top_new);	
			top_old = top_new;
		}
		handle_time();
		top_new = lua_gettop(pl);
		if(top_old!=top_new) {
			thread_log::ref.write_c_log_err(pl, log_msg, "mytest after handle_time,%d,%d",top_old,top_new);	
			top_old = top_new;
		}
		hot_update();
		top_new = lua_gettop(pl);
		if(top_old!=top_new) {
			thread_log::ref.write_c_log_err(pl, log_msg, "mytest after hot_update,%d,%d",top_old,top_new);	
			top_old = top_new;
		}
	}
}

void thread_logic::handle_io()
{
	for(;;)
	{
		msg_recv* cg = q_recv_io.getfront();
		msg_recv* gg = q_disconnect.getfront();
		msg_recv *msg = 0;
		if(!cg && !gg){
			break;
		}
		else if(cg && !gg){
			msg = q_recv_io.pop();
		}
		else if(!cg && gg){
			msg = q_disconnect.pop();
		}
		else if(cg->step < gg->step){
			msg = q_recv_io.pop();
		}
		else if(gg->step < cg->step){
			msg = q_disconnect.pop();
		}
		else{
			assert(0);
		}

		int top_old = lua_gettop(pl);
		msg_ex::ref.msg_read = msg;
		lua_getglobal(pl, "handlerMsg");
		lua_pushnumber(pl, msg->fd);
		lua_pushnumber(pl, msg->id);
		int top_new = lua_gettop(pl);
		if((top_old+3)!=top_new) {
			thread_log::ref.write_c_log_err(pl, log_msg, "mytest doing handle_io,%d,%d",top_old,top_new);
			sleep1;
		}
		if(lua_pcall(pl, 2, 0, errfunc)){
			const char *p = lua_tostring(pl, -1);
			DEBUG_PUTS(p);
			thread_log::ref.write_c_log_logic(pl, log_msg, p);
			lua_pop(pl, 1);
		}
		global::ref.msg_recv_pool.free(msg);		
		
	}
}

void thread_logic::handle_inner()
{

	for(;;)
	{
		msg_inner* msg = global::ref.q_inner_recv.pop();
		if(!msg)
		{
			break;
		}
		int top_old = lua_gettop(pl);
		msg_inner_parse::ref.resetRead(msg);
		lua_getglobal(pl, "handlerInner");
		lua_pushnumber(pl, msg->fd);
		int top_new = lua_gettop(pl);
		if((top_old+2)!=top_new) {
			thread_log::ref.write_c_log_err(pl, log_msg, "mytest doing handle_inner,%d,%d",top_old,top_new);
		}
		if(lua_pcall(pl, 1, 0, errfunc)){
			const char *p = lua_tostring(pl, -1);
			DEBUG_PUTS(p);
			thread_log::ref.write_c_log_logic(pl, log_msg, p);
			lua_pop(pl, 1);
			
		}
		global::ref.msg_inner_pool.free(msg);
	}

}

void thread_logic::handle_time()
{
	int top_old = lua_gettop(pl);
	lua_getglobal(pl, "handlerTime");
	lua_pushnumber(pl, global::ref.get_msec());
	int top_new = lua_gettop(pl);
	if((top_old+2)!=top_new) {
		thread_log::ref.write_c_log_err(pl, log_msg, "mytest doing handle_time,%d,%d",top_old,top_new);
	}
	if(lua_pcall(pl, 1, 0, errfunc)){
		const char *p = lua_tostring(pl, -1);
		DEBUG_PUTS(p);
		thread_log::ref.write_c_log_logic(pl, log_time, p);
		lua_pop(pl, 1);
	}
}

void thread_logic::handle_admin()
{
	for(;;){
		msg_admin* msg = q_recv_admin.pop();
		if (!msg) return;

		short fd = msg->fd;
		const char *p = 0;
		size_t nLen = 0;

		int top_old = lua_gettop(pl);

		// amin´¦Àí
		lua_getglobal(pl, "handlerAdmin");
		lua_pushnumber(pl,fd);
		lua_pushlstring(pl, msg->buf, msg->buf_len);
		int top_new = lua_gettop(pl);
		if((top_old+3)!=top_new) {
			thread_log::ref.write_c_log_err(pl, log_msg, "mytest doing handle_admin,%d,%d",top_old,top_new);
		}
		if(lua_pcall(pl, 2, 1, errfunc)){
			const char *p = lua_tostring(pl, -1);
			DEBUG_PUTS(p);
			thread_log::ref.write_c_log_logic(pl, log_msg, p);
		}else {
			p = lua_tolstring(pl, -1, &nLen);
		}
		lua_pop(pl, 1);
		global::ref.admin_recv_pool.free(msg);

		// ·µ»Ø
		if(p){
			msg_admin * msgSend = global::ref.admin_send_pool.alloc();
			if (!msgSend) {
				thread_log::ref.write_c_log_logic(pl, log_admin, "q_send_admin getback full");
				assert(0);
				continue;
			}

			msgSend->buf_len = 0;
			msgSend->fd = fd;
			msgSend->write_string(p, nLen);
			q_send_admin.push(msgSend);
		}
	}
}



void thread_logic::hot_update()
{
	if(!thread_hot::ref.need_hot(hot_time) ){
		return;
	}
	int top_old = lua_gettop(pl);
	int top_new = top_old;
	long long msec = global::ref.get_msec();
	if(luaL_loadfile(pl, global::ref.logic_main_file_name)){
		const char *p = lua_tostring(pl, -1);
		DEBUG_PUTS(p);
		thread_log::ref.write_c_log_logic(pl, log_hot, p);
		lua_pop(pl, 1);
		top_new = lua_gettop(pl);
		if((top_old)!=top_new) {
			thread_log::ref.write_c_log_err(pl, log_msg, "mytest doing hot_update luaL_loadfile err,%d,%d",top_old,top_new);
		}
		return;
	}
	if(lua_pcall(pl, 0, 0, errfunc)){
		const char *p = lua_tostring(pl, -1);
		DEBUG_PUTS(p);
		thread_log::ref.write_c_log_logic(pl, log_hot, p);
		lua_pop(pl, 1);
		top_new = lua_gettop(pl);
		if((top_old)!=top_new) {
			thread_log::ref.write_c_log_err(pl, log_msg, "mytest doing hot_update lua_pcall err,%d,%d",top_old,top_new);
		}
		return;
	}
	top_new = lua_gettop(pl);
	if((top_old)!=top_new) {
		thread_log::ref.write_c_log_err(pl, log_msg, "mytest doing hot_update ok,%d,%d",top_old,top_new);
	}
}
