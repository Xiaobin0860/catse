#include"global.h"
#include"thread_hot.h"
#include"thread_log.h"
#include"thread_logic.h"
#include"thread_http.h"
#include"thread_loop.h"
#include"thread_io.h"
#include"thread_inner.h"
#include"thread_admin.h"
#include"login_check.h"

lua_config_reader lua_config_reader::ref;
global global::ref;
thread_hot thread_hot::ref;
thread_log thread_log::ref;
thread_http thread_http::ref;
thread_io thread_io::ref;
websocket websocket::ref;
thread_inner thread_inner::ref;
thread_admin thread_admin::ref;
login_check login_check::ref;

int main(int argc, char **argv){
#ifdef __VERSION__
	for(int i = 1; i < argc; ++i){
		if(!strcmp(argv[i], "--config") && i + 1 < argc){
			global::ref.config_file_name = argv[i + 1];
			++i;
			continue;
		}
		if(!memcmp(argv[i], "--config=", 9) && argv[i][9]){
			global::ref.config_file_name = argv[i] + 9;
		}
	}
#else
	for(int i = 1; i < argc; ++i){
		if(!strcmp(argv[i], "--config") && i + 1 < argc){
			global::ref.config_file_name = argv[i + 1];
			++i;
			continue;
		}
		if(!memcmp(argv[i], "--config=", 9) && argv[i][9]){
			global::ref.config_file_name = argv[i] + 9;
		}
	}
#endif
#ifdef __VERSION__
	// daemon(1,1);
#else
	system("chcp 65001");
#endif
	global::ref.init();
	thread_io::ref.init();
	thread_log::ref.init();
	thread_http::ref.init();
	thread_inner::ref.init();
	thread_logic::ref.init();
	thread_admin::ref.init();
	
	//login_check::ref.login_check_http();
	//login_check::ref.start_check_run();
	
	global::ref.create_thread(&thread_log::ref);
	global::ref.create_thread(&thread_hot::ref);
	global::ref.create_thread(&thread_inner::ref);
	global::ref.create_thread(&thread_logic::ref);
	global::ref.create_thread(&thread_io::ref);
	global::ref.create_thread(&thread_http::ref);
	global::ref.create_thread(&thread_loop::ref);
	global::ref.create_thread(&thread_admin::ref);

	lua_close(lua_config_reader::ref.pl);

	puts("engine start ... ");

	for(;;){
		sleep1;
	}
	return 0;
}
