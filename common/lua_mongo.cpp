#include "global.h"
#include "lua_mongo.h"

static short hash_little_to_big[256]={0};
static char hash_big_to_little[256 * 256];



static void hash_init(){
	for(int i = 0; i < 256; ++i){
		sprintf((char*)(hash_little_to_big + i), "%02x", i);
		hash_big_to_little[(unsigned short)hash_little_to_big[i]] = i;
	}
}

void oid_init(oid* the, int id){
	if(hash_little_to_big[255] == 0)
	{
		hash_init();
	}
#ifdef __VERSION__
	short pid = getpid();
	const char split = ':';
	const char*str_sscanf = "%x:%x:%x";
	int ret = system("ifconfig>mac.txt");
	if(ret){
		printf("oid_init:%d:%s\n", ret, strerror(errno));
		assert(0);
		exit(0);
	}
#else
	short pid = GetCurrentProcessId();
	const char split = '-';
	const char*str_sscanf = "%x-%x-%x";

	if(system("ipconfig/all>mac.txt")){

		assert(0);
		exit(0);
	}
#endif
	FILE*file = fopen("mac.txt", "rb");
	if(!file){
		assert(0);
		exit(0);
	}
	char buf[1024];
	int buf_len = fread(buf, 1, sizeof(buf), file);
	if(buf_len < 1){
		assert(0);
		exit(0);
	}
	if(fclose(file)){
		assert(0);
		exit(0);
	}
	int i;
	for(i = 0; i + 12 < buf_len; ++i){
		int j;
		for(j = 0; j < 5; ++j){
			if(buf[i + j * 3] - split){
				break;
			}
		}
		if(j == 5){
			int mac[3];
			if(buf_len < i + 32){
				assert(0);
				exit(0);
			}
			if(3 - sscanf(buf + i + 7, str_sscanf, mac, mac + 1, mac + 2)){
				assert(0);
				exit(0);
			}
			the->mac[0] = mac[0];
			the->mac[1] = mac[1];
			the->mac[2] = mac[2];
			break;
		}
	}
	if(buf_len <= i + 12){
		assert(0);
		exit(0);
	}
	pid +=id;
	const char*p = (const char*)&pid;
	the->pid[0] = p[1];
	the->pid[1] = p[0];
}
void oid_gen_next(oid* the){
	int t = time(0);
	const char*p = (const char*)&t;
	the->time[0] = p[3];
	the->time[1] = p[2];
	the->time[2] = p[1];
	the->time[3] = p[0];
	static int inc;
	++inc;
	p = (const char*)&inc;
	the->inc[0] = p[2];
	the->inc[1] = p[1];
	the->inc[2] = p[0];
}

void little_to_big(char *out, const char *p, const int len){
	for(int i = 0; i < len; ++i){
		*(short*)(out + (i << 1)) = hash_little_to_big[(unsigned char)p[i]];
	}
}

void big_to_little(char *out, const char *p){
	for(int i = 0; i < 12; ++i){
		out[i] = hash_big_to_little[*(unsigned short*)(p + (i << 1))];
	}
}

oid g_oid;


lua_mongo lua_mongo::ref;
lua_State *lua_mongo::pl;
char lua_mongo::buf[];
int lua_mongo::buf_len;
int lua_mongo::buf_len_client;
int lua_mongo::buf_len_find;
int lua_mongo::buf_len_next;