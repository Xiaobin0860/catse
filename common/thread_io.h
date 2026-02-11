#pragma once
#include  "lz77.h"
#include  "pk_limit.h"
#include "websocket.h"
struct thread_io{
	static const int DISCONNECT_REASON_CLIENT = 1;		//client主动断开
	static const int DISCONNECT_REASON_TIMEOUT = 2;		//长时间没有发包断开
	static const int DISCONNECT_REASON_PACKET_ERR = 3;	//发送非法包断开
	static const int DISCONNECT_REASON_FD_ERR = 4;		//fd越界
	static const int DISCONNECT_REASON_USE_WPE = 5;		//使用wpe作弊
	static const int DISCONNECT_REASON_COME_ON_ERR = 6;	//
	event ev_time;
	event_base *eb;
	static const int fd_max = 32768;
	struct bufferevent *fd2be[fd_max];
	char fd2wpe[fd_max];
	char fd2websocket[fd_max];
	int fds_len;
	short fds[fd_max];
	int log_io_cg_full;
	int log_io_gc_full;
	msg_queue<msg_send> q_send;

	void cb_event(bufferevent *be, short nEvents){
		close_connect(be, nEvents & BEV_EVENT_TIMEOUT ? DISCONNECT_REASON_TIMEOUT : DISCONNECT_REASON_CLIENT,nEvents);
	}
	void cb_listener(int fd){// 新连接接入
		bufferevent *be = bufferevent_socket_new(eb, fd, BEV_OPT_CLOSE_ON_FREE);
		if(!be){
			assert(0);
			return;
		}
		if(!fd_add(fd, be)){
			assert(0);
			return;
		}
		fd2wpe[fd] = 0;
		fd2websocket[fd] =0 ;
		bufferevent_setwatermark(be, EV_READ, sizeof(msg_head), 0);
		timeval tv = {300, 0}; 
		bufferevent_set_timeouts(be, &tv, 0); // 300秒无数据就主动断开连接
		bufferevent_setcb(be, static_cb_read, 0, static_cb_event, this);
		bufferevent_enable(be, EV_READ);	
		int on = 1;
		setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char*)&on, sizeof(on));
	}
	void cb_read(bufferevent *be){
		evbuffer *in = bufferevent_get_input(be);

/*		static const char policy[] = "<policy-file-request/>";
		if (evbuffer_get_length(in) == sizeof(policy)){
			char buf[sizeof(policy)];
			evbuffer_copyout(in, &buf, sizeof(policy));
			if (!memcmp(policy, buf, sizeof(policy))){
				evbuffer_drain(in, sizeof(policy));
				static const char cross_domain_protocol[] = "<cross-domain-policy><allow-access-from domain=\"*\"to-ports=\"*\"/></cross-domain-policy>";
				bufferevent_write(be, cross_domain_protocol, sizeof(cross_domain_protocol));
				return;
			}
		}
*/
		int fd = bufferevent_getfd(be);
		if(!(0<=fd && fd < sizeof(fd2wpe))){
			assert(0);
			close_connect(be, DISCONNECT_REASON_FD_ERR, 0);
			return;
		}

		/* 腾讯以前服务器需要加这个才能访问
		if(!fd2comeon[fd]){
			static const char pre[] = "\r\nhost:";
			if(sizeof(pre) < evbuffer_get_length(in)){
				char buf[64];
				int len = evbuffer_get_length(in);
				if(sizeof(buf) < len){
					len = sizeof(buf);
				}
				evbuffer_copyout(in, buf, len);
				if(!memcmp(pre, buf, sizeof(pre) - 1)){
					const char *p = (const char *)memchr(buf + sizeof(pre), 10, len - sizeof(pre));
					if(p){
						fd2comeon[fd] = 1;
						evbuffer_drain(in, p - buf + 1);
					}else{
						if(len == sizeof(buf)){
							close_connect(be, DISCONNECT_REASON_COME_ON_ERR,0);
						}
						return;
					}
				}
			}
		}*/

		if (!fd2websocket[fd]) {
			bool ret = websocket::ref.hand_shake(be, in);
			if (ret == true) {
				fd2websocket[fd] = 1;	
			}
			return;
		}


		long long cur_time = 0;
		// msg_head长4 WebSocket数据帧头部长6，取大的6
		while(6 <= evbuffer_get_length(in)){
			int packet_length = websocket::ref.handle_packet(be, in);
			if (packet_length == 0) {
				continue;
			} else if (packet_length == -1) {
				close_connect(be, DISCONNECT_REASON_CLIENT, 0);
				return;
			} else if (packet_length == -2) {//预防没收完的情况
				break;
			}

			msg_head head;
			
			memcpy(&(head.len), websocket::ref.buf, sizeof(short));
			memcpy(&(head.id), websocket::ref.buf + sizeof(short), sizeof(short));
			head.id = ntohs(head.id);
			head.len = ntohs(head.len);

			if(!(0 <= head.len && head.len <= sizeof(((msg_recv*)0)->buf))){
				close_connect(be, DISCONNECT_REASON_PACKET_ERR,0);
				return;
			}
			if(packet_length != head.len + sizeof(head)){
				return;
			}
			if(!fd2be[fd]){
				assert(0);
				return;
			}
			if(!cur_time){
				cur_time = global::ref.get_msec();
			}
			if(pk_limit::ref.check_pk_limit(cur_time, fd, head.id) != pk_limit::err_ok){
#ifndef __VERSION__
				printf("pklimit:fd=%d,id=%d,name=%s\n", fd, head.id, pk_limit::ref.get_name(head.id));
#endif
				//evbuffer_drain(in, head.len + sizeof(head));
				continue;
			}

			msg_recv *msg = global::ref.msg_recv_pool.alloc();
			if(!msg){
				char buf[512];
				sprintf(buf, "%d,%d,%d,%s", fds_len, fd, head.id, pk_limit::ref.get_name(head.id));
				thread_log::ref.write_c_log_io(log_io_cg_full, buf);
				assert(0);
				return;
			}
			if(sizeof(msg->buf) < head.len || head.len < 0) {
				//回收
				global::ref.msg_recv_pool.free(msg);
				return;
			}
			
			memcpy(msg->buf, websocket::ref.buf + sizeof(head), head.len);
			msg->back = head.len;
			msg->front = 0;
			msg->fd = fd;
			msg->id = head.id;
			msg->set_step();
			thread_logic::ref.q_recv_io.push(msg);
		}
	}

	void cb_time(){
		long long cur_time = 0;
		for(;;){
			msg_send *msg = q_send.pop();
			if(!msg){
				break;
			}
/*			char a[65536];
			int a_len;
			bool f = lz77::zip(a, a_len, sizeof(a), msg->buf, msg->buf_len);
			if(!f){
				assert(0);
				global::ref.msg_send_pool.free(msg);
				break;
			}
*/
			if(!msg->fds_len){
				assert(0);
				global::ref.msg_send_pool.free(msg);
				break;
			}
			if(msg->fds_len < 0){
				// 世界广播
				for(int i = 0; i < fds_len; ++i){
					send_msg(fds[i], msg->id, msg->buf, msg->buf_len);
				}
			}
			else{
				// 针对特定fd广播
				for(int i = 0; i < msg->fds_len; ++i){
					send_msg(msg->fds[i], msg->id, msg->buf, msg->buf_len);
				}
			}
			global::ref.msg_send_pool.free(msg);
		}
		if(!cur_time){
			cur_time = global::ref.get_msec();
		}		
	}
	void close_connect(bufferevent *be, char reason, short real_reason){
		int fd = bufferevent_getfd(be);
		fd_del(fd);
		bufferevent_free(be);
		msg_recv *msg = global::ref.msg_recv_pool.alloc();
		if(!msg){
			assert(0);
			return;
		}
		msg->back = 0;
		msg->front = 0;
		msg->write_int(reason);
		msg->write_int(real_reason);
		msg->fd = fd;
		msg->id = global::ref.disconnect_id;
		msg->set_step();
		thread_logic::ref.q_disconnect.push(msg);
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
		sin.sin_port = htons(global::ref.client_port);

		q_send.init(global::ref.msg_send_pool.cap, false);

		evconnlistener *listener = evconnlistener_new_bind(eb, static_cb_listener, this, LEV_OPT_REUSEABLE, -1, (sockaddr*)&sin, sizeof(sin));
		if(!listener){
			puts("err evconnlistener_new_bind");
			assert(0);
			exit(0);
			return;
		}
		int n = event_assign(&ev_time, eb, -1, EV_PERSIST, static_cb_time, this);
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
		log_io_cg_full = thread_log::ref.reg("log/err_cg_full", "sum_fd, fd, id, name", 0, false);
	}
	void run(){
		event_base_dispatch(eb);
	}
	void send_msg(int fd, int id, const char *p, const int len){
		if(!(0<=fd && fd < sizeof(fd2be) / sizeof(*fd2be))){
			assert(0);
			return;
		}
		if(!fd2be[fd]){
			return;
		}
	
		bufferevent *be = fd2be[fd];
		websocket::ref.send_msg(be, id, p, len);
	}
	static void static_cb_event(bufferevent *be, short what, void *p){
		((thread_io*)p)->cb_event(be, what);
	}
	static void static_cb_listener(evconnlistener *, evutil_socket_t fd, sockaddr*, int, void *p){
		((thread_io*)p)->cb_listener(fd);
	}
	static void static_cb_read(bufferevent *be, void *p){
		((thread_io*)p)->cb_read(be);
	}
	static void static_cb_time(evutil_socket_t, short, void *p){
		((thread_io*)p)->cb_time();
	}
	static thread_io ref;
};