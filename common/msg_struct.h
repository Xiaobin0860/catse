#pragma once

#define TYP_INIT 0
#define TYP_SMLE 1
#define TYP_BIGE 2

struct msg_utils {
	static double cvt(double src) {
		static int typ = TYP_INIT;
		unsigned char c;
		union {
			double ull;
			unsigned char c[8];
		} x;

		if (typ == TYP_INIT) {
			x.ull = 0x01;
			typ = (x.c[7] == 0x01) ? TYP_BIGE : TYP_SMLE;
		}

		if (typ == TYP_BIGE) return src;

		x.ull = src;
		c = x.c[0]; x.c[0] = x.c[7]; x.c[7] = c;
		c = x.c[1]; x.c[1] = x.c[6]; x.c[6] = c;
		c = x.c[2]; x.c[2] = x.c[5]; x.c[5] = c;
		c = x.c[3]; x.c[3] = x.c[4]; x.c[4] = c;
		return x.ull;
	}

	static double htonll( double n )
	{
		return cvt(n);
	}

	static double ntohll( double n )
	{
		return cvt(n);
	}
};

struct msg_head{
    short len;
	short id;
};

struct msg_recv{
	char buf[512];
	short back;
	short front;
	short id;
	short fd;
	long long step;
	char get_last_char_read(){
		if(front){
			return buf[front - 1];
		}
		return 0;
	}
	char has_read_all(){
		return back == front;
	}
	char read_byte(){
		if(back<front+1){
			back = -1;
			return 0;
		}

		char ret = 0;
		memcpy(&ret, buf + front, 1);
		front = front + 1;
		return ret;
	}

	short read_short(){
		if(back<front+2){
			back = -1;
			return 0;
		}

		short ret = 0;
		memcpy(&ret, buf + front, 2);
		front = front + 2;
		return ntohs(ret);
	}
	int read_int(){
		if(back<front+4){
			back = -1;
			return 0;
		}

		int ret = 0;
		memcpy(&ret, buf + front, 4);
		front = front + 4;
		return ntohl(ret);
	}
	double read_double(){
		if(back<front+8){
			back = -1;
			return 0;
		}

		double ret = 0;
		memcpy(&ret, buf + front, 8);
		front = front + 8;
		return msg_utils::ntohll(ret);
	}
	int read_string(char *str, const int len){
		if(len < 0){
			back = -1;
			return 0;
		}
		if(len<0 || back < front + len){
			back = -1;
			return 0;
		}
		memcpy(str, buf + front, len);
		front += len;
		return len;
	}
	void set_step(){
		static long long s_step;
		step = ++s_step;
	}
	void write_int(unsigned n){
		if(sizeof(buf) < back+4){
			assert(0);
			return;
		}

		n = htonl(n);
		memcpy(buf + back, &n, 4);
		back = back + 4;
	}
	static char static_get_last_char_read(void *p){
		return ((msg_recv*)p)->get_last_char_read();
	}
	static char static_has_read_all(void *p){
		return ((msg_recv*)p)->has_read_all();
	}
	static char static_read_byte(void *p){
		return ((msg_recv*)p)->read_byte();
	}
	static int static_read_int(void *p){
		return ((msg_recv*)p)->read_int();
	}
	static short static_read_short(void *p){
		return ((msg_recv*)p)->read_short();
	}
	static double static_read_double(void *p){
		return ((msg_recv*)p)->read_double();
	}
	static void static_read_string(void *p, char *str, const int len){
		((msg_recv*)p)->read_string(str, len);
	}
};

struct msg_send{
	char buf[16384];
	short buf_len;
	short id;
	short fds[2048];
	short fds_len;
	long long time;

	char *get_last_char_write(){
		return buf + buf_len - 1;
	}
	void push_fd(int fd){
		if((sizeof(fds) / sizeof(*fds)) <= fds_len){
			assert(0);
			return;
		}
		fds[fds_len++] = fd;
	}
	void write_byte(char n){
		if(sizeof(buf) < buf_len+1){
			assert(0);
			return;
		}

		memcpy(buf + buf_len, &n, 1);
		buf_len = buf_len + 1;
	}
	void write_short(short n){
		if(sizeof(buf) < buf_len+2){
			assert(0);
			return;
		}

		n = htons(n);
		memcpy(buf + buf_len, &n, 2);
		buf_len = buf_len + 2;
	}

	void write_double(double nn){		
		if(sizeof(buf) < buf_len + 8){
			assert(0);
			return;
		}
		//lua里的0.1传到c++变成了0.099999999999999992.这里做一下四舍五入	
//		unsigned long long n = (unsigned long long)(roundM(nn*1000));

		nn = msg_utils::htonll(nn);
		memcpy(buf + buf_len, &nn, 8);
		buf_len = buf_len + 8;
		
	}
	void write_int(unsigned n){
		if(sizeof(buf) < buf_len+4){
			assert(0);
			return;
		}

		n = htonl(n);
		memcpy(buf + buf_len, &n, 4);
		buf_len = buf_len + 4;
	}
	void write_string(const char *str, const int len){
		if(sizeof(buf) < buf_len + len){
			assert(0);
			return;
		}
		memcpy(buf + buf_len, str, len);
		buf_len += len;
	}
	static char* static_get_last_char_write(void *p){
		return ((msg_send*)p)->get_last_char_write();
	}
	static void static_write_byte(void *p, char n){
		((msg_send*)p)->write_byte(n);
	}
	static void static_write_short(void* p, short n ){
		((msg_send*)p)->write_short(n);
	}
	static void static_write_double(void *p, double n){
		((msg_send*)p)->write_double(n);
	}
	static void static_write_int(void *p, unsigned n){
		((msg_send*)p)->write_int(n);
	}
	static void static_write_string(void *p, const char *str, const int len){
		((msg_send*)p)->write_string(str, len);
	}
};


struct msg_admin{
	char buf[65536 * 8];
	int buf_len;
	short fd;
	void write_string(const char *str, const int len){
		if(sizeof(buf) < buf_len + len){
			assert(0);
			return;
		}
		memcpy(buf + buf_len, str, len);
		buf_len += len;
	}
};

struct msg_http {
	char buf[65536 * 8];
	char urlBuf[65536];
	int buf_len;
	int url_buf_len;
	short fd;
	void write_string(const char *str, const int len){
		if(sizeof(buf) < buf_len + len){
			assert(0);
			return;
		}
		memcpy(buf + buf_len, str, len);
		buf_len += len;
	}
	void write_url_string(const char *str, const int len){
		if(sizeof(urlBuf) < url_buf_len + len){
			assert(0);
			return;
		}
		memcpy(urlBuf + url_buf_len, str, len);
		url_buf_len += len;
	}
};

//world和logic通信结构体
struct msg_inner{
	char  buf[204800];//200k 128k
	unsigned int buf_len;
	unsigned short fd;
	bool write_string(const char *str, const int len){
		if(sizeof(buf) < buf_len + len){
			assert(0);
			return false;
		}
		memcpy(buf + buf_len, str, len);
		buf_len += len;
		return true;
	}

	bool write_int(int d){
		if(sizeof(buf) < buf_len + sizeof(d)){
			assert(0);
			return false;
		}
		memcpy(buf + buf_len, &d, sizeof(d));
		buf_len += sizeof(d);
		return true;
	}
};
