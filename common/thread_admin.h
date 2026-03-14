/*管理后台的连接io处理线程.*/
#pragma once

struct thread_admin{
	event ev_time;
	event_base *eb;
	static const int fd_max = 32768;
	struct bufferevent *fd2be[fd_max];
	int fds_len;
	short fds[fd_max];

	void cb_event(bufferevent *be, short nEvents){
		close_connect(be);
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
		while(sizeof(((msg_admin*)0)->buf_len) <= evbuffer_get_length(in)){
			msg_admin *msg = global::ref.admin_recv_pool.alloc();
			if(!msg){
				assert(0);
				return;
			}
			evbuffer_copyout(in, &msg->buf_len, sizeof(msg->buf_len));
			if(!(0 <= msg->buf_len && msg->buf_len <= sizeof(((msg_admin*)0)->buf))){
				close_connect(be);
				global::ref.admin_recv_pool.free(msg);
				return;
			}
			if(evbuffer_get_length(in) < msg->buf_len + sizeof(msg->buf_len)){
				global::ref.admin_recv_pool.free(msg);
				return;
			}
			int fd = bufferevent_getfd(be);
			evbuffer_drain(in, sizeof(msg->buf_len));
			evbuffer_remove(in, msg->buf, msg->buf_len);
			msg->fd = fd;
			thread_logic::ref.q_recv_admin.push(msg);
		}
	}
	void cb_time(){
		for(;;){
			msg_admin *msg = thread_logic::ref.q_send_admin.pop();
			if(!msg){
				return;
			}
			int fd = msg->fd;
			if(!(0<=fd && fd < sizeof(fd2be) / sizeof(*fd2be))){
				assert(0);
				global::ref.admin_send_pool.free(msg);
				return;
			}
			if(!fd2be[fd]){
				global::ref.admin_send_pool.free(msg);
				return;
			}
			bufferevent *be = fd2be[fd];
			bufferevent_write(be, &msg->buf_len, sizeof(msg->buf_len));
			if(msg->buf_len){
				bufferevent_write(be, msg->buf, msg->buf_len);
			}
			global::ref.admin_send_pool.free(msg);
		}
	}
	void close_connect(bufferevent *be){
		int fd = bufferevent_getfd(be);
		fd_del(fd);
		bufferevent_free(be);
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
			assert(0);
			return false;
		}
		if(!fd2be[fd]){
			assert(0);
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
		eb = event_base_new();
		if(!eb){
			assert(0);
			exit(0);
			return;
		}
		sockaddr_in sin = {};
		sin.sin_family = AF_INET;
		lua_config_reader *reader = &lua_config_reader::ref;
		if(!reader->open(global::ref.config_file_name)){
			assert(0);
			exit(0);
			return;
		}
		int n;
		if(!reader->read_int("PORT_ADMIN", &n)){
			assert(0);
			exit(0);
			return;
		}
		sin.sin_port = htons(short(n));

		evconnlistener *listener = evconnlistener_new_bind(eb, static_cb_listener, this, LEV_OPT_REUSEABLE, -1, (sockaddr*)&sin, sizeof(sin));
		if(!listener){
			puts("err evconnlistener_new_bind");
			assert(0);
			exit(0);
			return;
		}
		n = event_assign(&ev_time, eb, -1, EV_PERSIST, static_cb_time, this);
		if(n){
			assert(0);
			exit(0);
			return;
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
		((thread_admin*)p)->cb_event(be, what);
	}
	static void static_cb_listener(evconnlistener *, evutil_socket_t fd, sockaddr*, int, void *p){
		((thread_admin*)p)->cb_listener(fd);
	}
	static void static_cb_read(bufferevent *be, void *p){
		((thread_admin*)p)->cb_read(be);
	}
	static void static_cb_time(evutil_socket_t, short, void *p){
		((thread_admin*)p)->cb_time();
	}
	static thread_admin ref;
};