#pragma once
struct thread_http{
	bufferevent *be;
	event_base *eb;
	event ev;
	char connect_state;
	char ip[64];
	char ip_len;
	sockaddr_in si;
	char urlencode[256][4];
	int log_id;

	msg_queue<msg_http> q_http_send;
	void cb_event(short what){
//		printf("cb_event %d\n", what);
		if(what&BEV_EVENT_CONNECTED){
			connect_state =  2;
		}
		else{
			bufferevent_free(be);
			be = 0;
			while(q_http_send.getfront()){
				msg_http *msg = q_http_send.pop();
				global::ref.http_send_pool.free(msg);
			}
		}
	}
	void cb_read(){
		evbuffer *in = bufferevent_get_input(be);
		int len = evbuffer_get_length(in);
//		printf("len=%d\n", len);
		char buf[8192] = {};
		if(len < sizeof(buf)){
			evbuffer_copyout(in, buf, len);
			buf[len] = 0;
#ifndef __VERSION__
			puts(buf);
#endif
		}
		else{
			assert(0);
		}
		evbuffer_drain(in, len);
	}
	void cb_time(){
		msg_http *m = q_http_send.getfront();
		if(!m){
			return;
		}
		if(!be){
			be = bufferevent_socket_new(eb, -1, BEV_OPT_CLOSE_ON_FREE);
			if(!be){
				assert(0);
				return;
			}
			bufferevent_setcb(be, static_cb_read, 0, static_cb_event, 0);
			bufferevent_enable(be, EV_READ);
			connect_state = 0;
		}
		if(!connect_state){
			if(bufferevent_socket_connect(be, (sockaddr*)&si, sizeof(si))){
				assert(0);
				return;
			}
			connect_state = 1;
			return;
		}
		if(connect_state == 1){
			return;
		}

		m = q_http_send.pop();
		if(!m->buf_len){
			assert(0);
			global::ref.http_send_pool.free(m);
			return;
		}

		static const char aa[] = "GET ";
		static const char bb[] = " HTTP/1.1\nHost:";
		static const char cc[] = "\n\n";
		char buf[sizeof(m->buf)];
		if(sizeof(buf)<sizeof(aa) + sizeof(bb) + sizeof(cc) + m->buf_len + m->url_buf_len + ip_len){
			assert(0);
			global::ref.http_send_pool.free(m);
			return;
		}
		int buf_len = 0;
		for(int i = 0; i < m->buf_len; ++i){
			unsigned char t = m->buf[i];
			if(urlencode[t][0]){
				if(sizeof(buf) < buf_len + 3){
					assert(0);
					global::ref.http_send_pool.free(m);
					return;
				}
				memcpy(buf + buf_len, urlencode[t], 3);
				buf_len += 3;
			}
			else{
				if(sizeof(buf) <= buf_len){
					assert(0);
					global::ref.http_send_pool.free(m);
					return;
				}
				buf[buf_len++] = m->buf[i];
			}
		}
		memcpy(m->buf, buf, buf_len);
		m->buf_len = buf_len;
		if(sizeof(buf)<sizeof(aa) + sizeof(bb) + sizeof(cc) + m->buf_len + m->url_buf_len + ip_len){
			assert(0);
			global::ref.http_send_pool.free(m);;
			return;
		}
		buf_len = 0;
		memcpy(buf + buf_len, aa, sizeof(aa) - 1);buf_len += sizeof(aa) - 1;
		memcpy(buf + buf_len, m->urlBuf, m->url_buf_len);buf_len += m->url_buf_len;
		memcpy(buf + buf_len, m->buf, m->buf_len);buf_len += m->buf_len;
		memcpy(buf + buf_len, bb, sizeof(bb) - 1);buf_len += sizeof(bb) - 1;
		memcpy(buf + buf_len, ip, ip_len);buf_len += ip_len;
		memcpy(buf + buf_len, cc, sizeof(cc) - 1);buf_len += sizeof(cc) - 1;
		bufferevent_write(be, buf, buf_len);
		global::ref.http_send_pool.free(m);
	}
	void init(){
		for(int i = 0; i < 256; ++i){
			if(!('0'<=i && i <= '9' || 
				'A'<=i && i <= 'Z' ||
				'a'<=i && i <= 'z'))
			sprintf(urlencode[i], "%%%02X", i);
		}
		log_id = thread_log::ref.reg("log/err_http_full", "", 0, false);
		lua_config_reader *reader = &lua_config_reader::ref;
		if(!reader->open(global::ref.config_file_name)){
			assert(0);
			exit(0);
		};
		int n;
		if(!reader->read_string("IP_HTTP", ip, sizeof(ip))){
			assert(0);
			exit(0);
		}
		ip_len = strlen(ip);
		int port;
		if(!reader->read_int("PORT_HTTP", &port)){
			assert(0);
			exit(0);
		}
		si.sin_family = AF_INET;
		si.sin_addr.s_addr = inet_addr(ip);
		si.sin_port = htons(port);

		eb = event_base_new();
		if(!eb){
			assert(0);
			exit(0);
		}
		q_http_send.init(global::ref.http_send_pool.cap + 1, false);
		event_assign(&ev, eb, -1, EV_PERSIST, static_cb_time, this);
		timeval tv = {0, 1};
		event_add(&ev, &tv);
	}
	void register_lua(lua_State *pl){
		luaL_Reg l[] = {
			{"send", static_send},
			{0, 0},
		};
		global::ref.reg_tb_func(pl, l, "thread_http");
	}
	void run(){
		event_base_dispatch(eb);
	}
	void send(lua_State *pl){
		int top_old = lua_gettop(pl);
		size_t urllen = 0;
		const char *urlp = luaL_checklstring(pl, 1, &urllen);
		size_t len = 0;
		const char *p = luaL_checklstring(pl, 2, &len);
		msg_http *m = global::ref.http_send_pool.alloc();
		if(!m){
			thread_log::ref.write_c_log_logic(pl, log_id, p);
			return;
		}

		m->buf_len = 0;
		m->write_string(p, len);
		m->url_buf_len = 0;
		m->write_url_string(urlp, urllen);
		q_http_send.push(m);
		int top_new = lua_gettop(pl);
		if(top_old!=top_new) {
			luaL_error(pl, "mytest thread_http.send top_old=%d,top_new=%d", top_old,top_new);		
		}
	}
	static void static_cb_event(bufferevent*, short what, void *arg){
		ref.cb_event(what);
	}
	static void static_cb_read(bufferevent*, void *arg){
		ref.cb_read();
	}
	static void static_cb_time(evutil_socket_t, short, void *arg){
		ref.cb_time();
	}
	int static static_send(lua_State *pl){
		ref.send(pl);
		return 0;
	}
	static thread_http ref;
};

