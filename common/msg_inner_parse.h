#pragma once

#include "msg_struct.h"
#include "thread_inner.h"


struct msg_inner_parse {

	static msg_inner_parse ref;

	msg_inner *msg_read;

	int offset_read;

	msg_inner * msg_send;

	bson bWrite;

	bson bRead;

	char bufRead[65536 * 8];
	//压缩空间.
	char zipBuf[65536 * 8];
	//解压内存空间.
	char unzipBuf[65536 * 8];

	msg_inner_parse() {
		resetRead(0);
		msg_send = NULL;
	}

	void resetRead(msg_inner *msgInput) {
		msg_read = msgInput;
		offset_read = 0;
	}

	void register_lua(lua_State *pl){
		luaL_Reg l[] = {
			{ "readint", static_read_int },
			{ "readstring", static_read_string },
			{ "readtable", static_read_table },
			{ "readchunk", static_read_chunk },
			{ "readbinary", static_read_binary },
			{ "readzip", static_read_zip },
			{ "unbinary", static_read_unbinary },

			{ "writebegin", static_write_begin },
			{ "writeint", static_write_int },
			{ "writestring", static_write_string },
			{ "writetable", static_write_table },
			{ "writebinary", static_write_binary },
			{ "writechunk", static_write_chunk },
			{ "writezip", static_write_zip },
			{ "writeend", static_write_end },

			{ "check_connect", static_check_connect },
			{ "close_connect", static_close_connect },
			{ "set_connect", static_set_connect },
			{ 0, 0 }
		};
		global::ref.reg_tb_func(pl, l, "msg_inner_parse");
	}

	int read_int(lua_State *pl) {
		if (msg_read == 0) {
			lua_pushboolean(pl, 0);
			return 1;
		}
		if (offset_read + sizeof(int) > msg_read->buf_len) {
			lua_pushboolean(pl, 0);
			return 1;
		}

		int n = 0;
		memcpy(&n, msg_read->buf + offset_read, sizeof(int));
		offset_read = offset_read + sizeof(int);

		lua_pushboolean(pl, 1);
		lua_pushinteger(pl, n);
		return 2;
	}

	int read_string(lua_State *pl) {
		if (msg_read == 0) {
			lua_pushboolean(pl, 0);
			return 1;
		}
		if (offset_read + sizeof(int) > msg_read->buf_len) {
			lua_pushboolean(pl, 0);
			return 1;
		}

		int n = 0;
		memcpy(&n, msg_read->buf + offset_read, sizeof(int));
		offset_read = offset_read + sizeof(int);

		if (offset_read + n > msg_read->buf_len) {
			lua_pushboolean(pl, 0);
			return 1;
		}

		if (n < 0 || n > sizeof(bufRead)) {
			lua_pushboolean(pl, 0);
			return 1;
		}

		memset(bufRead, 0, sizeof(bufRead));
		memcpy(bufRead, msg_read->buf + offset_read, n);
		offset_read = offset_read + n;

		lua_pushboolean(pl, 1);
		lua_pushlstring(pl, bufRead, n);
		return 2;
	}

	int read_table(lua_State *pl) {
		if (msg_read == 0) {
			lua_pushboolean(pl, 0);
			return 1;
		}

		if (offset_read + sizeof(int) > msg_read->buf_len) {
			lua_pushboolean(pl, 0);
			return 1;
		}

		int n = 0;
		memcpy(&n, msg_read->buf + offset_read, sizeof(int));
		offset_read = offset_read + sizeof(int);

		if (offset_read + n > msg_read->buf_len) {
			lua_pushboolean(pl, 0);
			return 1;
		}

		if (n < 0 || n > sizeof(bufRead)) {
			lua_pushboolean(pl, 0);
			return 1;
		}

		memset(bufRead, 0, sizeof(bufRead));
		memcpy(bufRead, msg_read->buf + offset_read, n);
		offset_read = offset_read + n;

		bson_destroy(&bRead);
		bson_init(&bRead);
		bson_finish(&bRead);
		bson_init_finished_data(&bRead, bufRead, 0);
		
		lua_mongo::ref.bson2table(bson_data(&bRead), pl, false);

		lua_pushboolean(pl, 1);
		return 1;
	}

	int read_chunk(lua_State *pl) {
		if (msg_read == 0) {
			lua_pushboolean(pl, 0);
			return 1;
		}
		if (offset_read + sizeof(int) > msg_read->buf_len) {
			lua_pushboolean(pl, 0);
			return 1;
		}

		int n = 0;
		memcpy(&n, msg_read->buf + offset_read, sizeof(int));
		offset_read = offset_read + sizeof(int);

		if (offset_read + n > msg_read->buf_len) {
			lua_pushboolean(pl, 0);
			return 1;
		}

		if (n < 0 || n > sizeof(bufRead)) {
			lua_pushboolean(pl, 0);
			return 1;
		}

		memset(bufRead, 0, sizeof(bufRead));
		memcpy(bufRead, msg_read->buf + offset_read, n);
		offset_read = offset_read + n;

		lua_pushboolean(pl, 1);
		lua_pushlstring(pl, bufRead, n);
		return 2;
	}

	int read_binary(lua_State *pl) {
		if (msg_read == 0) {
			lua_pushboolean(pl, 0);
			return 1;
		}

		if (offset_read + sizeof(int) > msg_read->buf_len) {
			lua_pushboolean(pl, 0);
			return 1;
		}

		// 读取头长度.
		int n = 0;
		memcpy(&n, msg_read->buf + offset_read, sizeof(int));
		offset_read = offset_read + sizeof(int);

		if (offset_read + n > msg_read->buf_len) {
			lua_pushboolean(pl, 0);
			return 1;
		}

		if (n < 0 || n > sizeof(bufRead)) {
			lua_pushboolean(pl, 0);
			return 1;
		}

		memset(bufRead, 0, sizeof(bufRead));
		memcpy(bufRead, msg_read->buf + offset_read, n);
		offset_read = offset_read + n;

		// 解压内存.
		int zipBufLen(0);
		bool zipRes = lz77::unzip(unzipBuf, zipBufLen, sizeof(unzipBuf), bufRead, n);
		if (!zipRes){
			lua_pushboolean(pl, 0);
			return 1;
		}
		bson_destroy(&bRead);
		bson_init(&bRead);
		bson_finish(&bRead);
		bson_init_finished_data(&bRead, unzipBuf, 0);

		lua_mongo::ref.bson2table(bson_data(&bRead), pl, false);

		lua_pushboolean(pl, 1);
		return 1;
	}

	int read_unbinary(lua_State *pl){
		const char * p = luaL_checkstring(pl, 1);
		if (p == 0) {
			lua_pushboolean(pl, 0);
			return 1;
		}

		int len = luaL_checkint(pl, 2);
		if (len == 0) {
			lua_pushboolean(pl, 0);
			return 1;
		}
		memset(bufRead, 0, sizeof(bufRead));
		memcpy(bufRead, p, len);

		// 解压内存.
		int zipBufLen(0);
		bool zipRes = lz77::unzip(unzipBuf, zipBufLen, sizeof(unzipBuf), bufRead, len);
		if (!zipRes){
			lua_pushboolean(pl, 0);
			return 1;
		}
		bson_destroy(&bRead);
		bson_init(&bRead);
		bson_finish(&bRead);
		bson_init_finished_data(&bRead, unzipBuf, 0);

		lua_mongo::ref.bson2table(bson_data(&bRead), pl, false);

		lua_pushboolean(pl, 1);
		return 1;
	}

	int read_zip(lua_State *pl){
		if (msg_read == 0) {
			lua_pushboolean(pl, 0);
			return 1;
		}

		if (offset_read + sizeof(int) > msg_read->buf_len) {
			lua_pushboolean(pl, 0);
			return 1;
		}

		// 读取头长度.
		int n = 0;
		memcpy(&n, msg_read->buf + offset_read, sizeof(int));
		offset_read = offset_read + sizeof(int);

		if (offset_read + n > msg_read->buf_len) {
			lua_pushboolean(pl, 0);
			return 1;
		}

		if (n < 0 || n > sizeof(bufRead)) {
			lua_pushboolean(pl, 0);
			return 1;
		}

		memset(bufRead, 0, sizeof(bufRead));
		memcpy(bufRead, msg_read->buf + offset_read, n);
		offset_read = offset_read + n;

		// 解压内存.
		int zipBufLen(0);
		bool zipRes = lz77::unzip(unzipBuf, zipBufLen, sizeof(unzipBuf), bufRead, n);
		if (!zipRes){
			lua_pushboolean(pl, 0);
			return 1;
		}
		bson_destroy(&bRead);
		bson_init(&bRead);
		bson_finish(&bRead);
		bson_init_finished_data(&bRead, unzipBuf, 0);
		lua_mongo::ref.bson2table(bson_data(&bRead), pl, false);
		lua_pushboolean(pl, 1);
		return 1;
	}

	int write_string(lua_State *pl) {
		const char * p = luaL_checkstring(pl, 1);
		if (p == 0) {
			luaL_error(pl, "write string err1");
		}

		if (msg_send->write_int(strlen(p)) == false) {
			luaL_error(pl, "write string err2");
		}
		if (msg_send->write_string(p, strlen(p)) == false) {
			luaL_error(pl, "write string err3");
		}
		return 0;
	}

	int write_table(lua_State *pl) {
		bson_destroy(&bWrite);
		bson_init(&bWrite);
		lua_mongo::ref.table2bson(pl, &bWrite, false);
		bson_finish(&bWrite);
		int bsonSize = bson_size(&bWrite);
		if (msg_send->write_int(bsonSize) == false) {
			luaL_error(pl, "write table err1");
		}
		if (msg_send->write_string(bWrite.data, bsonSize) == false) {
			luaL_error(pl, "write table err2");
		}
		return 0;
	}

	int write_chunk(lua_State *pl) {

		bson_destroy(&bWrite);
		bson_init(&bWrite);
		lua_mongo::ref.table2bson(pl, &bWrite, false);
		bson_finish(&bWrite);
		int bsonSize = bson_size(&bWrite);

		int zipBufLen(0);
		bool zipRes = lz77::zip(zipBuf, zipBufLen, sizeof(zipBuf), bWrite.data, bsonSize);
		if (!zipRes){
			return 0;
		}
		if (msg_send->write_int(zipBufLen) == false) {
			luaL_error(pl, "write table err1");
		}

		if (msg_send->write_string(zipBuf, zipBufLen) == false) {
			luaL_error(pl, "write table err2");
		}

		return 0;
	}

	int write_binary(lua_State *pl) {
		const char * p = luaL_checkstring(pl, 1);
		if (p == 0) {
			luaL_error(pl, "write string err1");
			return 0;
		}

		int len = luaL_checkint(pl, 2);
		if (len == 0) {
			return 0;
		}

		if (msg_send->write_int(len) == false) {
			luaL_error(pl, "write table err1");
		}

		if (msg_send->write_string(p, len) == false) {
			luaL_error(pl, "write table err2");
		}
		return 0;
	}

	int write_zip(lua_State *pl){
		bson_destroy(&bWrite);
		bson_init(&bWrite);
		lua_mongo::ref.table2bson(pl, &bWrite, false);
		bson_finish(&bWrite);
		int bsonSize = bson_size(&bWrite);

		int zipBufLen(0);
		bool zipRes = lz77::zip(zipBuf, zipBufLen, sizeof(zipBuf), bWrite.data, bsonSize);
		if (!zipRes){
			assert(0);
			return 0;
		}
		if (msg_send->write_int(zipBufLen) == false) {
			luaL_error(pl, "write table err1");
		}

		if (msg_send->write_string(zipBuf, zipBufLen) == false) {
			luaL_error(pl, "write table err2");
		}
		return 0;
	}

	static int static_read_int(lua_State *pl){
		return ref.read_int(pl);
	}

	static int static_read_string(lua_State *pl)
	{
		return ref.read_string(pl);
	}

	static int static_read_table(lua_State *pl){
		return ref.read_table(pl);
	}

	static int static_read_chunk(lua_State *pl){
		return ref.read_chunk(pl);
	}

	static int static_read_binary(lua_State *pl){ 
		return ref.read_binary(pl); 
	}

	static int static_read_zip(lua_State *pl){
		return ref.read_zip(pl);
	}

	static int static_read_unbinary(lua_State *pl){
		return ref.read_unbinary(pl);
	}

	static int static_write_begin(lua_State *pl){
		if (!ref.msg_send)
		{
			ref.msg_send = global::ref.msg_inner_pool.alloc();
		}
		if (!ref.msg_send){
			luaL_error(pl, "");
		}
		ref.msg_send->fd = (unsigned short)luaL_checknumber(pl, 1);
		ref.msg_send->buf_len = 0;
		if (!ref.msg_send->write_int(luaL_checknumber(pl, 2))){
			global::ref.msg_inner_pool.free(ref.msg_send);
			ref.msg_send = NULL;
			luaL_error(pl, "write_id error");
		}
		return 0;
	}

	static int static_write_int(lua_State *pl){
		if (ref.msg_send == 0 || !ref.msg_send->write_int(luaL_checknumber(pl, 1))){
			luaL_error(pl, "write int err");
		}
		return 0;
	}

	static int static_write_string(lua_State *pl){
		if (ref.msg_send == 0){
			luaL_error(pl, "");
		}
		return ref.write_string(pl);
	}

	static int static_write_table(lua_State *pl){
		if (ref.msg_send == 0){
			luaL_error(pl, "");
		}
		return ref.write_table(pl);
	}

	static int static_write_chunk(lua_State *pl){
		if (ref.msg_send == 0){
			luaL_error(pl, "");
		}
		return ref.write_chunk(pl);
	}

	static int static_write_binary(lua_State *pl){
		if (ref.msg_send == 0){
			luaL_error(pl, "");
		}
		return ref.write_binary(pl);
	}

	static int static_write_zip(lua_State *pl){
		if (ref.msg_send == 0){
			luaL_error(pl, "");
		}
		return ref.write_zip(pl);
	}

	static int static_write_end(lua_State *pl){
		if (ref.msg_send == 0){
			luaL_error(pl, "");
		}
		global::ref.q_inner_send.push(ref.msg_send);
		ref.msg_send = NULL;
		return 0;
	}

	static int static_check_connect(lua_State *pl){
		if (thread_inner::ref.isMiddle == true) {
			lua_pushboolean(pl, 1);
			return 1;
		}

		if (thread_inner::ref.connect_state == 2) {
			lua_pushboolean(pl, 1);
		}
		else {
			lua_pushboolean(pl, 0);
			//thread_inner::ref.needConnect = true;
		}
		return 1;
	}

	static int static_close_connect(lua_State *pl){
		thread_inner::ref.forceClose = true;
		return 0;
	}

	static int static_set_connect(lua_State *pl){
		thread_inner::ref.set_connect(pl);
		return 0;
	}
};
msg_inner_parse msg_inner_parse::ref;
