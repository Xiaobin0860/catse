#ifndef __WEBSOCKET_REQUEST__
#define __WEBSOCKET_REQUEST__


class Websocket_Request {
public:
	Websocket_Request();
	~Websocket_Request();
	int fetch_websocket_info(char *msg);
	void reset();

private:
	int fetch_fin(char *msg, int &pos);
	int fetch_opcode(char *msg, int &pos);
	int fetch_mask(char *msg, int &pos);
	int fetch_masking_key(char *msg, int &pos);
	int fetch_payload_length(char *msg, int &pos);
	int fetch_payload(char *msg, int &pos);
public:
	unsigned char fin_;
	unsigned char opcode_;
	unsigned char mask_;
	unsigned char masking_key_[4];
	int payload_length_;
	char payload_[2048];
};

#endif
