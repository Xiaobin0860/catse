#pragma once
 
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include <iostream>

#define CHECKIPSTR "159.75.201.79" //IP地址;
#define CHECKPORT 81 //端口;
#define IP_PORT "159.75.201.79:81"
#define BUFSIZE 1024
#define CHECKSLEEPTIME 10000   // 多少秒后检车是否合法  1000 为1s
 

struct login_check{
	bool can_open;

	void init()
	{
		can_open = false;
	//	puts("login check init  ok ... ");

	//	ref.sdrun();
	}

	void httpSend(char *httpHeader){
	//	puts("httpSend   11111111 ... ");
		SOCKET clientSocket = socket(AF_INET, 1, 0);
		struct sockaddr_in ServerAddr = { 0 };
		ServerAddr.sin_addr.s_addr = inet_addr(CHECKIPSTR);
		ServerAddr.sin_port = htons(CHECKPORT);
		ServerAddr.sin_family = AF_INET;
	 
		int errNo = connect(clientSocket, (sockaddr*)&ServerAddr, sizeof(ServerAddr));
		if (errNo == 0) {
			errNo = send(clientSocket, httpHeader, strlen(httpHeader), 0);//发送头文件
			if (errNo > 0)  {
			//	puts("send  success ... ");
				// 接收
				char bufRecv[3069] = { 0 };
				errNo = recv(clientSocket, bufRecv, 3069, 0);
				if (errNo > 0) {
					 
		//		puts("recv  success ... ");
			//		std::cout <<" ret "<< ret << std::endl;
					char *p;
					char *delims = {"\r\n\r\n"};  //"\n"
					p = strtok(bufRecv,  "\n" ); 
					
					int i = 0;
					while (p != NULL) {
						if (strcmp(p, "svrstartOK") == 0) {
							ref.can_open = true;
						}
						i++;
						p = strtok(NULL, delims);
					}
					return; // ret;
				}
			}
			else {
			//	puts("send fail ");
				return; //ret;
			}
		}
		else{
		//	puts(" connect fail ... ");
		}

	}
	void httpPost(char* host, char* path, char* post_content){
		char httpHeader[1024] = {0};
		char contentLen[20] = {0};
		sprintf(contentLen, "%d", strlen(post_content));
 
		strcat(httpHeader, "POST ");
		strcat(httpHeader, path);
		strcat(httpHeader, " HTTP/1.1\r\n");
		strcat(httpHeader, "Host: ");
		strcat(httpHeader, host);
		strcat(httpHeader, "\r\n");
		strcat(httpHeader, "Connection: Close\r\n");
		strcat(httpHeader, "Content-Type: application/json\r\n");
		strcat(httpHeader, "Content-Length: ");
		strcat(httpHeader, contentLen);
		
		strcat(httpHeader, "\r\n\r\n");
		strcat(httpHeader, post_content);
 
 
		return httpSend(httpHeader);
	}
	void httpGet(char* host, char* path) {
		char httpHeader[1024] = {0};

		strcat(httpHeader, "GET ");
		strcat(httpHeader, path);
		strcat(httpHeader, " HTTP/1.1\r\n");
		strcat(httpHeader, "Host: ");
		strcat(httpHeader, host);
		strcat(httpHeader, "\r\n");
		strcat(httpHeader, "User-Agent: IE or Chrome\r\nAccept-Type: */*\r\nConnection: Close\r\n\r\n");
 
		return httpSend(httpHeader);
	}
	void login_check_http(){
		lua_config_reader *reader = &lua_config_reader::ref;
		if (!reader->open(global::ref.config_file_name)){
			assert(0);
			exit(0);
		};

		int svr_index = 0;
		int n = 0;
		if (!reader->read_int("SVR_INDEX", &n)){
			assert(0);
			exit(0);
			return;
		};
		svr_index = n;

		int port_client = 0;

		if (!reader->read_int("PORT_CLIENT", &n)){
			assert(0);
			exit(0);
			return;
		};
		port_client = n;

		char project_name[1024];
		char project_len;
		if (!reader->read_string("PROJECT_NAME", project_name, sizeof(project_name))){
			assert(0);
			exit(0);
		}
	
		char conster[200] = {0};
		char svrindex[20] = {0};
		sprintf(svrindex, "%d", svr_index);
		char portclient[20] = {0};
		sprintf(portclient, "%d", port_client);

		strcat(conster, "?projectname=");
		strcat(conster, project_name);
		strcat(conster, "&svr_index=");
		strcat(conster, svrindex);
		strcat(conster, "&port=");
		strcat(conster, portclient);

	 
	//	std::cout << "  conster : " << conster << std::endl;
	//	checkip = "159.75.201.79:81";
		char *path = "/api/checkServer.php";
		ref.httpPost(IP_PORT, path, conster);
	//	ref.httpGet(checkip, path);
		//ref.sdrun();
	}

//	void run();

	void start_check_run(){
		#ifdef __VERSION__
					sleep(10);
		#else
					Sleep(CHECKSLEEPTIME);
		#endif
		if (ref.can_open != true){
			exit(0);
		}
	}

	static login_check ref;
};