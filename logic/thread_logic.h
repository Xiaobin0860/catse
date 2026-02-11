#pragma once


struct thread_logic
{
	int log_hot;			// 热更新错误日志文件id
	int log_msg;			// 消息处理错误日志文件id
	int log_time;			// 定时器处理错误日志文件id
	int log_admin;			//
		

	msg_queue<msg_recv>  q_recv_io;			//从io传来的消息
	msg_queue<msg_recv>  q_disconnect;		

	msg_queue<msg_admin>  q_recv_admin;		//
	msg_queue<msg_admin>  q_send_admin;		//

	lua_State *pl;
	int errfunc;			// lua出错后的回调函数


	long long hot_time;

	int tid;


	void init();
	void handle_io();
	void handle_inner();
	void handle_time();
	void handle_admin();
	void hot_update();
	void run();

	/*
	static int server_info(lua_State* pl)
	{
		lua_pushnumber(pl, global::ref.msg_send_pool.min_top);
		lua_pushnumber(pl, global::ref.msg_recv_pool.min_top);
		lua_pushnumber(pl, global::ref.msg_inner_pool.min_top);
		return 3;
	}
	*/
	static int static_get_thread_id(lua_State* pl)
	{
		lua_pushnumber(pl, ref.tid);
		return 1;
	}
	static thread_logic ref;
};
