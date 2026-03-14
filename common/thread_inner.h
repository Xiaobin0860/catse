/*world 和logic通信线程.*/
#pragma once

struct thread_inner{
	event ev_time;
	event_base *eb;
	static const int fd_max = 32768;

	static const int PACKET_ID_HELLO = 1;
	static const int PACKET_ID_DISCONNECT_LW = 2;
	static const int PACKET_ID_HEARTBEAT = 3;
	static const int PACKET_ID_DISCONNECT_WL = 4;

	bool isMiddle;	// 是否跨服.

	// 跨服需要的变量.
	int fds_len;
	short fds[fd_max];
	struct bufferevent *fd2be[fd_max];

	// 正常服逻辑需要的变量.
	bufferevent *beNormal; 
	bool needConnect;
	bool forceClose;
	char connect_state;     // 0：无连接  1：连接中 2：已接上.
	sockaddr_in si;			// 连接地址.
	char ip[64];
	char ip_len;
	bool setIp;
	int svr_index;			// 服务器index

	void cb_event(bufferevent *be, short nEvents) {
		if (isMiddle) {
			// 正常服到跨服的连接断开 关闭连接.
			close_connect(be);
		}else {
			if(nEvents & BEV_EVENT_CONNECTED){
				// 链接跨服成功 发送hello
				connect_state =  2; // 連上了.
				sendHelloPacketToWorld();
			} else {
				// 其他情况断开连接.
				if(beNormal && connect_state == 2) close_connect(beNormal);
				else bufferevent_free(beNormal);
				beNormal = 0;
				connect_state = 0;
			}
		}
	}

	void cb_listener(int fd){
		// 新连接接入.
		bufferevent *be = bufferevent_socket_new(eb, fd, BEV_OPT_CLOSE_ON_FREE);
		if(!be){
			assert(0);
			return;
		}
		if(!fd_add(fd, be)){
			return;
		}

		bufferevent_setwatermark(be, EV_READ, sizeof(int), 0);
		timeval tv = {300, 0}; 
		bufferevent_set_timeouts(be, &tv, 0); // 300秒无数据就主动断开连接.
		bufferevent_setcb(be, static_cb_read, 0, static_cb_event, this);
		bufferevent_enable(be, EV_READ);
	}
	void cb_read(bufferevent *be){
		evbuffer *in = bufferevent_get_input(be);
		unsigned int buf_len;
		while(sizeof(((msg_inner*)0)->buf_len) <= evbuffer_get_length(in)){
			
			evbuffer_copyout(in, &buf_len, sizeof(buf_len));
			if(!(0 <= buf_len && buf_len <= sizeof(((msg_inner*)0)->buf))){
				close_connect(be);
				return;
			}
			if(evbuffer_get_length(in) < buf_len + sizeof(buf_len)){
				return;
			}
			msg_inner *msg = global::ref.msg_inner_pool.alloc();
			if(!msg){
				assert(0);
				return;
			}
			msg->buf_len = buf_len;
			int fd = bufferevent_getfd(be);
			evbuffer_drain(in, sizeof(msg->buf_len));
			evbuffer_remove(in, msg->buf, msg->buf_len);
			msg->fd = fd;
			global::ref.q_inner_recv.push(msg);
		}
	}
	void cb_time_logic(){
		if (forceClose == true) {
			forceClose = false;
			dis_connect();
			return;		
		}

		if (connect_state != 2) {
			if (needConnect == true) connect();
			return;
		}

		for(;;){

			if (connect_state != 2) {
				if (needConnect == true) connect();
				return;
			}

			msg_inner *msg = global::ref.q_inner_send.pop();
			if(!msg){
				return;
			}
			bufferevent_write(beNormal, &msg->buf_len, sizeof(msg->buf_len));
			if(msg->buf_len){
				bufferevent_write(beNormal, msg->buf, msg->buf_len);
			}
			global::ref.msg_inner_pool.free(msg);
		}
	}

	void cb_time_world(){
		for(;;){
			msg_inner *msg = global::ref.q_inner_send.pop();
			if(!msg){
				return;
			}
			
			short fd = msg->fd;
			if((0<=fd && fd < sizeof(fd2be) / sizeof(*fd2be))){
				if (fd2be[fd]) {
					bufferevent *be = fd2be[fd];
					bufferevent_write(be, &msg->buf_len, sizeof(msg->buf_len));
					if(msg->buf_len){
						bufferevent_write(be, msg->buf, msg->buf_len);
					}
				}
			}
			global::ref.msg_inner_pool.free(msg);
		}
	}

	void close_connect(bufferevent *be){
		int fd = bufferevent_getfd(be);
		fd_del(fd);
		bufferevent_free(be);

		msg_inner *msg = global::ref.msg_inner_pool.alloc();
		if(!msg){
			assert(0);
			return;
		}
		msg->fd = fd;
		msg->buf_len = 0;
		if(isMiddle){
			msg->write_int(PACKET_ID_DISCONNECT_LW);
		} else {
			msg->write_int(PACKET_ID_DISCONNECT_WL);
		}
		global::ref.q_inner_recv.push(msg);
	}
	void connect() {
		if (isMiddle) return; 
		if (connect_state != 0) return; 
		if (setIp == false) return;
		
		if (beNormal == 0) {
			beNormal = bufferevent_socket_new(eb, -1, BEV_OPT_CLOSE_ON_FREE);
			if(!beNormal){
				assert(0);
				return;
			}		
		}

		bufferevent_setcb(beNormal, static_cb_read, 0, static_cb_event, this);
		bufferevent_enable(beNormal, EV_READ);

		//printf("do connect\n");

		needConnect = false;
		if(bufferevent_socket_connect(beNormal, (sockaddr*)&si, sizeof(si))){
			assert(0);
			return;
		}
		connect_state = 1; // 連接中.
	}

	// 断开连接.
	void dis_connect(){
		if (beNormal == 0) return;

		if (beNormal && connect_state == 2) 
			close_connect(beNormal);
		else 
			bufferevent_free(beNormal);
		beNormal = 0;
		connect_state = 0;
	}

	// 设置连接.
	void set_connect(lua_State *pl){
		size_t str_len = 0;
		const char *str = luaL_checklstring(pl, 1, &str_len);
		int port = luaL_checknumber(pl, 2);

		si.sin_family = AF_INET;
		si.sin_port = htons(short(port));
		si.sin_addr.s_addr = inet_addr(str);
		setIp = true;
		forceClose = true;
		needConnect = true;
	}

	void sendHelloPacketToWorld() {
		int id = PACKET_ID_HELLO;
		int packetLen = sizeof(svr_index) + sizeof(id);
		bufferevent_write(beNormal, &packetLen, sizeof(packetLen));
		bufferevent_write(beNormal, &id, sizeof(id));
		bufferevent_write(beNormal, &svr_index, sizeof(svr_index));
	}

	bool fd_add(int fd, bufferevent *be){
		if(!(0<=fd && fd < sizeof(fd2be) / sizeof(*fd2be))){
			assert(0);
			return false;
		}
		if(fd2be[fd]){
			assert(0);
			return false;
		}
		for(int i = 0; i < fds_len; ++i){
			if(fds[i] == fd){
				assert(0);
				return false;
			}
		}
		if(fd_max <= fds_len){
			assert(0);
			return false;
		}
		fd2be[fd] = be;
		fds[fds_len++] = fd;
		return true;
	}


	bool fd_del(int fd){
		if(!(0<=fd && fd < sizeof(fd2be) / sizeof(*fd2be))){
			//assert(0);
			return false;
		}
		if(!fd2be[fd]){
			//assert(0);
			return false;
		}
		for(int i = 0; i < fds_len; ++i){
			if(fds[i] == fd){
				fd2be[fd] = 0;
				fds[i] = fds[--fds_len];
				return true;
			}
		}
		assert(0);
		return false;
	}


	void init(){
		eb = 0;
		isMiddle = global::ref.isMiddle;
		fds_len = 0; 
		beNormal = 0; 
		connect_state = 0;
		ip_len = 0;
		svr_index = 0;
		setIp = false;
		needConnect = false;
		forceClose = false;

		// eventBase初始化.
		eb = event_base_new();
		if(!eb){
			assert(0);
			exit(0);
			return;
		}

		lua_config_reader *reader = &lua_config_reader::ref;
		if(!reader->open(global::ref.config_file_name)){
			assert(0);
			exit(0);
			return;
		}

		// 正常服启动相关配置初始化.
		if(!reader->read_int("SVR_INDEX", &svr_index)){
			assert(0);
			exit(0);
			return;
		}

		/*if(!reader->read_string("MIDDLE_IP", ip, sizeof(ip))){
			assert(0);
			exit(0);
		}
		ip_len = strlen(ip);
		*/
		
		int port;
		if(!reader->read_int("MIDDLE_PORT", &port)){
			assert(0);
			exit(0);
			return;
		}

		int n;
		if (isMiddle) {
			sockaddr_in sin = {};
			sin.sin_family = AF_INET;
			sin.sin_port = htons(short(port));

			n = event_assign(&ev_time, eb, -1, EV_PERSIST, static_cb_time_world, this);
			if(n){
				assert(0);
				exit(0);
				return;
			}
			//跨服相关逻辑.
			evconnlistener *listener = evconnlistener_new_bind(eb, static_cb_listener, this, LEV_OPT_REUSEABLE, -1, (sockaddr*)&sin, sizeof(sin));
			if(!listener){
				puts("err evconnlistener_new_bind");
				assert(0);
				exit(0);
				return;
			}

		} else {

			/*si.sin_family = AF_INET;
			si.sin_port = htons(short(port));
			si.sin_addr.s_addr = inet_addr(ip);
			needConnect = true;*/

			n = event_assign(&ev_time, eb, -1, EV_PERSIST, static_cb_time_logic, this);
			if(n){
				assert(0);
				exit(0);
				return;
			}			
		}

		timeval tv = {0, 1};
		n = event_add(&ev_time, &tv);
		if(n){
			assert(0);
			exit(0);
			return;
		}
	}
	void run(){
		event_base_dispatch(eb);
	}
	static void static_cb_event(bufferevent *be, short what, void *p){
		((thread_inner*)p)->cb_event(be, what);
	}
	static void static_cb_listener(evconnlistener *, evutil_socket_t fd, sockaddr*, int, void *p){
		((thread_inner*)p)->cb_listener(fd);
	}
	static void static_cb_read(bufferevent *be, void *p){
		((thread_inner*)p)->cb_read(be);
	}
	static void static_cb_time_logic(evutil_socket_t, short, void *p){
		((thread_inner*)p)->cb_time_logic();
	}
	static void static_cb_time_world(evutil_socket_t, short, void *p){
		((thread_inner*)p)->cb_time_world();
	}
	static thread_inner ref;
};
