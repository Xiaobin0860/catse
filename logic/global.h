#pragma once 
#include<env.h>
#include<event2/buffer.h>
#include<event2/bufferevent.h>
#include<event2/event.h>
#include<event2/event_struct.h>
#include<event2/listener.h>
#include<event2/util.h>
#include "common.h"


#include<mongo.h>

#include"lua_config_reader.h"
#include"msg_pool.h"
#include"msg_queue.h"
#include"msg_struct.h"


struct global
{
	const char* config_file_name;
	const char* logic_main_file_name;

	msg_stack_pool<msg_recv>  msg_recv_pool;
	msg_stack_pool<msg_send>  msg_send_pool;
	msg_stack_pool<msg_inner> msg_inner_pool;

	msg_stack_pool<msg_admin>  admin_recv_pool;
	msg_stack_pool<msg_admin>  admin_send_pool;
	msg_stack_pool<msg_http>  http_send_pool;

	msg_queue<msg_inner> q_inner_recv;		//传来的消息
	msg_queue<msg_inner> q_inner_send;		//发去的消息

	bool isMiddle;		// 是否跨服

	unsigned short client_port;

	unsigned int  disconnect_id;

	template<typename T>int create_thread(T *pthread){
#ifdef __VERSION__
		pthread_t p;
		pthread_create(&p, 0, static_create_thread<T>, pthread);
		return p;
#else
		CreateThread(0, 0, (LPTHREAD_START_ROUTINE)static_create_thread<T>, pthread, 0, 0);
		return 0;
#endif
	}

	int get_thread_id(){
#ifdef __VERSION__
		return syscall(SYS_gettid);
#else
		return GetCurrentProcessId();
#endif
	}
	int get_file_last_modify_time(const char *filename){
#ifdef __VERSION__
		struct stat st = {};
		stat(filename, &st);
		return st.st_mtime;
#else
		WIN32_FIND_DATA wfd = {};
		HANDLE handle = FindFirstFile(filename, &wfd);
		FindClose(handle);
		return wfd.ftLastWriteTime.dwLowDateTime;
#endif
	}
	void get_ip_from_fd(lua_State *pl){
		sockaddr_in addr;
#ifdef __VERSION__	
		socklen_t nAddrLen = sizeof(addr);
#else	
		int nAddrLen = sizeof(addr);
#endif
		int n = getpeername((int)luaL_checknumber(pl, 1), (sockaddr*)&addr, &nAddrLen);
		if(n){
			lua_pushstring(pl, "");
			return;
		}
		lua_pushstring(pl, inet_ntoa(addr.sin_addr));
	}
	long long get_msec(){
		timeval tv;
		evutil_gettimeofday(&tv, 0);
		return (tv.tv_sec * 1000000LL + tv.tv_usec) / 1000;
	}
	long long get_usec(){
#ifdef __VERSION__	
		timeval tv;
		evutil_gettimeofday(&tv, 0);
		return tv.tv_sec * 1000000LL + tv.tv_usec;
#else	
		LARGE_INTEGER liQPF;
		LARGE_INTEGER time;
		QueryPerformanceFrequency(&liQPF);
		QueryPerformanceCounter(&time);
		return 1000000 * time.QuadPart / liQPF.QuadPart;
#endif
	}
	void init(){
		
		config_file_name = "script/Config.lua";
		logic_main_file_name = "script/Main.lua";

		// 读取lua配置文件
		lua_config_reader *reader = &lua_config_reader::ref;
		if(!reader->open(config_file_name)){
			assert(0);
			exit(0);
		}

		isMiddle = reader->read_bool("IS_MIDDLE");

		long long total  = 0;
		int n;
		if(!reader->read_int("PORT_CLIENT", &n)){
			assert(0);
			exit(0);
			return;
		}
		client_port = n;

		if(!reader->read_int("DISCONNECT_PROTO_ID", &n)){
			assert(0);
			exit(0);
			return;
		}
		disconnect_id = n;

		if(!reader->read_int("Q_CAP_MSG_DISCONNECT", &n)){
			assert(0);
			exit(0);
			return;
		}
		int nn;
		if(!reader->read_int("Q_CAP_MSG_RECV", &nn)){
			assert(0);
			exit(0);
		}
		n += nn;
		msg_recv_pool.init(n);
		total += n* sizeof(msg_recv);

		if(!reader->read_int("Q_CAP_MSG_SEND", &n)){
			assert(0);
			exit(0);
		}
		msg_send_pool.init(n);
		total += n*sizeof(msg_send);
		if(!reader->read_int("Q_CAP_MSG_INNER", &n)){
			assert(0);
			exit(0);
		}		
		msg_inner_pool.init(n);
		total += n*sizeof(msg_inner);

		q_inner_recv.init(n, false);
		q_inner_send.init(n, false);

		if(!reader->read_int("Q_CAP_MSG_ADMIN", &n)){
			assert(0);
			exit(0);
		}
		http_send_pool.init(n*0.5);
		admin_recv_pool.init(n);
		admin_send_pool.init(n);

		total += 2*0.5* sizeof(msg_http);

		fprintf(stderr, "logic all pool mem size:%lldM\n",total/1024/1024);

		

		// 初始化mongodb连接相关
		mongo_env_sock_init();
#ifdef __VERSION__
		struct sigaction sa;
		sa.sa_handler = SIG_IGN;
		sigaction( SIGPIPE, &sa, 0 );
#endif
	}
	void reg_tb_func(lua_State *pl, luaL_Reg*l, const char *tb_name){
		int len = 0;
		lua_newtable(pl);
		for(; l->name; ++l){
			lua_pushcfunction(pl,l->func);
			lua_setfield(pl, -2, l->name);
		}
		lua_setglobal(pl, tb_name);
	}
	void register_lua(lua_State *pl){
		luaL_Reg l[] = {
			{"date", static_date},
			{"get_file_last_modify_time", static_get_file_last_modify_time},
			{"get_ip_from_fd", static_get_ip_from_fd},
			{"get_msec", static_get_msec},
			{"get_usec", static_get_usec},
			{0, 0}
		};
		reg_tb_func(pl, l, "global");
	}
	
	template<typename T>static void* static_create_thread(void *pthread){
		((T*)pthread)->run();
		return 0;
	}
	static int static_date(lua_State *pl){
		time_t t = (time_t)luaL_checknumber(pl, 1);
		tm* p = localtime(&t);
		if(!p){
			luaL_error(pl, "date %ldd", t);
		}
		tm tt = *p;
		lua_pushnumber(pl, tt.tm_year + 1900);
		lua_pushnumber(pl, tt.tm_mon + 1);
		lua_pushnumber(pl, tt.tm_mday);
		lua_pushnumber(pl, tt.tm_hour);
		lua_pushnumber(pl, tt.tm_min);
		lua_pushnumber(pl, tt.tm_sec);
		lua_pushnumber(pl, tt.tm_wday + 1);
		lua_pushnumber(pl, tt.tm_yday + 1);
		return 8;
	}
	static int static_get_file_last_modify_time(lua_State *pl){
		lua_pushnumber(pl, ref.get_file_last_modify_time(luaL_checkstring(pl, 1)));
		return 1;
	}
	static int static_get_ip_from_fd(lua_State *pl){
		ref.get_ip_from_fd(pl);
		return 1;
	}
	static int static_get_msec(lua_State *pl){
		lua_pushnumber(pl, (double)ref.get_msec());
		return 1;
	}
	static int static_get_usec(lua_State *pl){
		lua_pushnumber(pl, (double)ref.get_usec());
		return 1;
	}
	static int static_traceback(lua_State *pl){
		luaL_traceback(pl, pl, lua_tostring(pl, -1), 0);
		return 1;
	}
	static global ref;
};
