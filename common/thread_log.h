/*写日志文件的线程.*/
#pragma once
#include "common.h"
#include"msg_queue.h"
extern tm threadLogLocalTime;


struct log_node{
	int id;
	char buf[8192];
	int buf_len;
};

struct log_info{
	FILE	*file;
	char	filename[128];
	int		filename_len;
	bool	has_data;
	int		div_time; //间隔.
	bool    is_json;  //是否json json则不加mtime mdate头.
	int		last_time_div;
	char	head[512]; //列名.

	void set_head(const char *head, const int headLen){
		if(sizeof(this->head) < headLen){
			assert(0);
			return;
		}
		memcpy(this->head, head, headLen);
	}

	void update_file(time_t curTime){
		char log[] = ".log";
		if(!div_time){
			if(file){
				return;
			}
			if(sizeof(filename) < filename_len + sizeof(log)){
				assert(0);
				return;
			}
			memcpy(filename + filename_len, log, sizeof(log));
			return;
		}
		if(curTime < last_time_div + div_time){
			return;
		}
		last_time_div = curTime;
		if(file){
			fclose(file);
			file = 0;
		}
		char bufTime[64];
		int bufTimeLen = strftime(bufTime, sizeof(bufTime), "_%Y-%m-%d-%H-%M", &threadLogLocalTime);
		if(bufTimeLen <= 0){
			assert(0);
			return;
		}
		if(sizeof(filename) < filename_len + bufTimeLen + sizeof(log)){
			assert(0);
			return;
		}
		memcpy(filename + filename_len, bufTime, bufTimeLen);
		memcpy(filename + filename_len + bufTimeLen, log, sizeof(log));
	}

	void flush(){
		if(!file){
			return;
		}
		if(has_data){
			has_data = false;
			fflush(file);
			return;
		}
	}
	void write(long long curTime, const char *buf, const int buf_len){
		update_file(curTime);
		if(!file){
			file = fopen(filename, "rb");
			bool is_new = !file;
			if(file){
				fclose(file);
			}
			file = fopen(filename, "ab");
			if(!file){
				assert(0);
				return;
			}
			if(is_new && head[1]){
				fwrite("mdate,mtime,", 1, sizeof("mdate,mtime,") - 1, file);
				fwrite(head, 1, strlen(head), file);
				char t = 10;
				fwrite(&t, 1, 1, file);
			}
		}
		time_t time_cur = time(0);
		char str_time[64] = {0};
		int str_time_len = strftime(str_time, sizeof(str_time), "%Y-%m-%d %H:%M:%S,", &threadLogLocalTime);
		if(!str_time_len){
			assert(0);
			return;
		}
		if(head[1]){
			int tmp = sprintf(str_time + str_time_len, "%d,", time_cur);
			str_time_len += tmp;
		}
		if (is_json == false) fwrite(str_time, 1, str_time_len, file);
		fwrite(buf, 1, buf_len, file);
		has_data = true;
	}
};

struct thread_log{

	message_queue<log_node>  q_io;
	message_queue<log_node>  q_list;    //



	log_info li[256];
	int li_len;
	char need_hot_log;
	char my_err_msg[100];

    
	void init(){
		lua_config_reader *reader = &lua_config_reader::ref;
		if(!reader->open("script/Config.lua")){
			assert(0);
			exit(0);
		}
		
		int n;
		if(!reader->read_int("Q_CAP_LOG_LOGIC", &n)){
			assert(0);
			exit(0);
			return;
		}
		q_list.init(n);
		q_io.init(8);
		
	}
	

	// 注册日志文件.
	int reg(const char *filename, const char *head, int divTime, bool isJson){
		if(sizeof(li)/sizeof(*li) <= li_len){
			assert(0);
			return 0;
		}
		int filename_len = strlen(filename);
		if(sizeof(li->filename) < filename_len + 1){
			assert(0);
			return 0;
		}
		for(int i = 0; i < li_len; ++i){
			// 判断日志文件是否已经被注册过.
			if(!memcmp(filename, li[i].filename, filename_len)){
				return i;
			}
		}
		memcpy(li[li_len].filename, filename, filename_len + 1);
		li[li_len].filename_len = filename_len;
		int head_len = strlen(head);
		if(sizeof(li->head) <= head_len) {
			assert(0);
			return 0;
		}
		memcpy(li[li_len].head, head, head_len);
		li[li_len].head[head_len] = 0;
		if(divTime < 0) {
			assert(0);
			return 0;
		}
		li[li_len].div_time = divTime;
		li[li_len].is_json = isJson;
		int ret = li_len++;
		return ret;
	}

	int reg(lua_State* pl){

		size_t fileNameLen = 0;
		const char* fileName = luaL_checklstring(pl, 1, &fileNameLen);
		size_t headLen = 0;
		const char* head = luaL_checklstring(pl, 2, &headLen);
		int divTime = (int)luaL_checknumber(pl, 3);
		bool isJson = lua_toboolean(pl, 4);

		lua_pushnumber(pl, reg(fileName, head, divTime, isJson));
		return 1;
	}

	// 写日志逻辑.
	void do_queue(message_queue<log_node>& q){
		while(q.getfront()){
			log_node *plog = q.getfront();
			if(0<=plog->id && plog->id<sizeof(li)/sizeof(*li)){
				time_t curTime = time(0);
				li[plog->id].write(curTime, plog->buf, plog->buf_len);
			}
			q.pop();
		}
	}

	void run(){
		for(unsigned step = 0; ; ++step){
			sleep1;
			// linux下面如果文件用别的工具修改后 存在无法继续写入的问题，所以热更新的时候 重新打开一下日志文件.
			if (need_hot_log){
				for(int i = 0; i < li_len; ++i){
					if(li[i].file){
						fclose(li[i].file);
						li[i].file = 0;
					}
				}
				need_hot_log = false;
			}
	
			do_queue(q_io);
			
			do_queue(q_list);

			if(step & 63){
				continue;
			}

			//每秒flush一次所有日志文件.
			for(int i = 0; i < li_len; ++i){
				li[i].flush();
			}
		}
	}
	void urgent_write(const char *reason, int logid, const char *msg, int len){
		FILE *file = fopen("log/err_urgent.log", "ab");
		if(!file){
			file = fopen("log/err_urgent2.log", "ab");
		}
		fwrite(reason, 1, strlen(reason), file);
		if(0<=logid && logid<li_len){
			fwrite(li[logid].filename, 1, strlen(li[logid].filename) + 1, file);
		}
		else{
			fwrite("logid error", 1, sizeof("logid error"), file);
		}
		fwrite(msg, 1, len, file);
		fclose(file);
	}
	int table_concat(char *buf, const int cap, bool isLog, int start, lua_State *pl){
		int top = (int)lua_gettop(pl);
		int bufLen = 0;
#define MY_MEMCPY(STR) if(cap -bufLen < sizeof(STR) - 1){return -1;}else{memcpy(buf + bufLen, STR, sizeof(STR) - 1); bufLen += sizeof(STR) - 1;}
#define MY_ADDCHAR(C) if(isLog){if(cap <= bufLen){return -1;}else{buf[bufLen++] = C;}}
		for(int i = start; i <= top; ++i){
			if(start < i){
				MY_ADDCHAR(',');
			}
			switch(lua_type(pl, i)){
	case LUA_TNIL:
		{
			MY_MEMCPY("(type):nil");
			break;
		}
	case LUA_TNUMBER:
		{
			double d = lua_tonumber(pl, i);
			int ret = 0;
			double tmp = d - (long long)d;
			if(-1e-8 < tmp && tmp < 1e-2){
				ret = snprintf(buf + bufLen, cap - bufLen - 1, "%lld", (long long)d);
			}
			else{
				ret = snprintf(buf + bufLen, cap - bufLen - 1, "%lf", d);
			}
			if(ret<=0){
				return -1;
			}
			bufLen += ret;
			break;
		}
	case LUA_TBOOLEAN:
		{
			if(lua_toboolean(pl, i)){
				MY_MEMCPY("true");
			}
			else{
				MY_MEMCPY("false");
			}
			break;
		}
	case LUA_TSTRING:
		{
			//MY_ADDCHAR('\"');
			size_t len = 0;
			const char *p = lua_tolstring(pl, i , &len);
			if(cap - bufLen < (int)len){
				return -1;
			}
			memcpy(buf + bufLen, p, len);
			bufLen += len;
			//MY_ADDCHAR('\"');
			break;
		}
	case LUA_TTABLE:
		{
			MY_MEMCPY("(type):table");
			break;
		}
	case LUA_TFUNCTION:
		{
			MY_MEMCPY("(type):function");
			break;
		}
	case LUA_TUSERDATA:
		{
			MY_MEMCPY("(type):userdata");
			break;
		}
	case LUA_TTHREAD:
		{
			MY_MEMCPY("(type):thread");
			break;
		}
	case LUA_TLIGHTUSERDATA:
		{
			MY_MEMCPY("(type):lightuserdata");
			break;
		}
	default:
		{
			luaL_error(pl, "error:%d", lua_type(pl, i));
			assert(0);
			break;
		}
#undef MY_MEMCPY
			}
		}
		MY_ADDCHAR(10);
		return bufLen;
	}
	int write(lua_State* pl){
		log_node *plog = q_list.getback();
		if(!plog){
			// 如果日志缓存队列爆了 立刻写入urgent文件中.
			int logid = (int)luaL_checknumber(pl, 1);
			char buf[8192] = {0};
			int buf_len = table_concat(buf, sizeof(buf) - 1, true, 2, pl);
			if(buf_len < 0){
				buf_len = sizeof(buf);
			}
			urgent_write("write_lua_log log null", logid, buf, buf_len);
			assert(0);
			return 0;
		}
		plog->id = (int)luaL_checknumber(pl, 1);
		plog->buf_len = table_concat(plog->buf, sizeof(plog->buf) - 1, true,2, pl);
		if(plog->buf_len < 0){
			urgent_write("write_lua_log buf_len exceed", plog->id, plog->buf, sizeof(plog->buf));
			assert(0);
			return 0;
		}
		q_list.push();
		return 0;
	}
	void write_c_log_io(int logid, const char*msg)
	{
		write_c_log(q_io, logid, msg);
	}

	void write_c_log_logic(lua_State* pl, int logid, const char* msg)
	{
		write_c_log(q_list, logid, msg);
	}
	void write_c_log_err(lua_State* pl, int logid, const char* msg, int top_old, int top_new)
	{
		sprintf(my_err_msg, msg, top_old, top_new);
		write_c_log(q_list, logid, my_err_msg);
	}
private:
	void write_c_log(message_queue<log_node> &q, int logid, const char *msg){
		log_node *plog = q.getback();
		if(!plog){
			urgent_write("write_c_log log null", logid, msg, strlen(msg));
			assert(0);
			return;
		}
		int msg_len = strlen(msg);
		if(sizeof(plog->buf) < msg_len + 1){
			urgent_write("write_c_log msgLen exceed", logid, msg, strlen(msg));
			assert(0);
			return;
		}
		plog->id = logid;
		memcpy(plog->buf, msg, msg_len);
		plog->buf[msg_len] = 10;
		plog->buf_len = msg_len + 1;
		q.push();
	}
public:

	void register_lua(lua_State *pl){
		luaL_Reg l[] = {
			{"reg", static_reg},
			{"write",static_write},
			{0, 0}
		};
		global::ref.reg_tb_func(pl, l, "lua_log");		
	}

	// lua接口：日志文件注册.
	static int static_reg(lua_State* pl){		
		return ref.reg(pl);
	}

	// lua接口：写日志文件.
	static int static_write(lua_State* pl){
		return ref.write(pl);
	}

	static thread_log ref;
};
