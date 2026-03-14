#pragma once
#include "sha1.h"
#include "base64.h"
#include "websocket_request.h"
struct websocket{
	char key[128];
	char buf[2048];
	Websocket_Request request;

	websocket() {
		memset(key, 0, sizeof(key));
		memset(buf, 0, sizeof(buf));
	}

	bool hand_shake(bufferevent *be, evbuffer *in) {
		static const char ret_str_1[] = "HTTP/1.1 101 Switching Protocols\r\nUpgrade:websocket\r\nConnection:Upgrade\r\nSec-WebSocket-Accept:";
		static const char ret_str_2[] = "\r\n\r\n";

		int len = evbuffer_get_length(in);
		if(sizeof(buf) < len){
			evbuffer_drain(in, len);
			assert(0);
			return false;
		}

		// 看是否收完了一个完整的http包.
		evbuffer_copyout(in, buf, len);
		if (memcmp(buf+len-4, ret_str_2, 4) != 0) {
			return false;
		}

		evbuffer_drain(in, len);
		// 从http包里面找key
		bool ret = get_key(buf);
		if (ret == false) {
			return false;
		}

		// 根据算法生成新key
		std::string server_key = key;
		SHA1 sha;
		unsigned int message_digest[5];
		sha.Reset();
		sha << server_key.c_str();
		sha.Result(message_digest);
		for (int i = 0; i < 5; i++) {
			message_digest[i] = htonl(message_digest[i]);
		}
		server_key = base64_encode(reinterpret_cast<const unsigned char*>(message_digest),20);

		// 回握手包.
		bufferevent_write(be, ret_str_1, strlen(ret_str_1));
		bufferevent_write(be, server_key.c_str(), server_key.length());
		bufferevent_write(be, ret_str_2, strlen(ret_str_2));
		return true;
	}


	// 获取websocket握手key
	bool get_key(char * buf) {
		static const char key_str_begin[] = "Sec-WebSocket-Key: ";
		static const char key_str_end[] = "\r\n";
		static const char magic_str[] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

		char *str1 = strstr(buf, key_str_begin);
		if (str1) {
			char *str2 = strstr(str1, key_str_end);
			memcpy(key, str1 + strlen(key_str_begin), str2 - str1 - strlen(key_str_begin));
			memcpy(key + (str2 - str1 - strlen(key_str_begin)), magic_str, strlen(magic_str));
			return true;
		}

		return false;
	}

	// 根据websocket协议发包.
	void send_msg(bufferevent *be, int id, const char *p, const int len) {
		static unsigned char socket_head[4] = {};
		int socket_head_len = 0;

		msg_head head = {len, id};
		int real_len = len + sizeof(msg_head);

		if (real_len < 126) {
			socket_head[0] = 130; //129是text 130是2进制.
			socket_head[1] = real_len;
			socket_head_len = 2;
		} else if (real_len < 65536) {
			socket_head[0] = 130; //129是text 130是2进制.
			socket_head[1] = 126;
			socket_head[2] = (unsigned char)(real_len >> 8 & 0xFF);
			socket_head[3] = (unsigned char)(real_len & 0xFF);
			socket_head_len = 4;
		} else {
			assert(0);
			return;
		}

		bufferevent_write(be, socket_head, socket_head_len);
		short tt = htons(head.len);
		bufferevent_write(be, &tt, 2);
		tt = htons(head.id);
		bufferevent_write(be, &tt, 2);
		//bufferevent_write(be, &head, sizeof(head));
		if (len > 0) {
			bufferevent_write(be, p, len);
		}
	}

	int handle_packet(bufferevent *be, evbuffer *in) {
		int len = evbuffer_get_length(in);
		if(sizeof(buf) < len){
			len = sizeof(buf);
		}
		evbuffer_copyout(in, buf, len);
		request.reset();
		int real_len = request.fetch_websocket_info(buf);
		if (len < real_len) {//如果收到的包小于实际的，还是再等等.
			return -2;
		}
		evbuffer_drain(in, real_len);

		if (request.fin_ != 1) {
			return -1;
		}

		if (request.opcode_ != 2) {
			if (request.opcode_ == 9 || request.opcode_ == 10) {
				return 0;
			} else {
				return -1;
			}
		}

		if (request.payload_length_ > 0) {
			memcpy(buf, request.payload_, request.payload_length_);
		}

		return request.payload_length_;
	}

	static websocket ref;
};

