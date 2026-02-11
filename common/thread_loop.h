#pragma once
struct thread_loop{
	long long run_times;
	long long run_last_times;
	lua_State* pl;
	bool isWorld;
	void init(lua_State* L, bool isWorld){

		run_times = 0;
		run_last_times = 0;

		pl = L;

		this->isWorld = isWorld;
	}
	void run(){
		for(;;){
#ifdef __VERSION__
			sleep(99);
#else
			Sleep(10000);
#endif
			if(run_times && run_last_times == run_times){					
				for(int i = 0; i < 16; ++i){
					lua_Debug ar;
					if(!lua_getstack(pl, i, &ar)){
						break;
					}
					lua_getinfo(pl, "Slnt", &ar);
					
					FILE*f ;
					if(isWorld){
						f = fopen("dumpw.txt", "ab");
					}else{
						f = fopen("dump.txt", "ab");
					}
					fprintf(f, "%s, %d\n", ar.short_src, ar.currentline);
					fclose(f);
				}
#ifdef __VERSION__
				exit(0);
#else
#endif
			}
			run_last_times = run_times;
		}
		
	}
	static thread_loop ref;
};

