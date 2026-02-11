/*热更新线程 检查main_file_name文件是否修改 如果修改 则需要热更新*/
#pragma once

struct thread_hot{   
	long long time_modify;	// 文件修改时间
	thread_hot()
	{
		time_modify = 0;
	}
	void run(){
		for(;;){
			for(int i = 0; i < 100; ++i){
				sleep1;
			}
			long long tmp = global::ref.get_file_last_modify_time(global::ref.logic_main_file_name);
			if(tmp - time_modify){
				if(time_modify){
				}
				time_modify = tmp;
			}
		}
	}

	bool need_hot(long long & t)
	{
		bool ret  = false;
		if(time_modify - t)
		{
			if(t)
			{
				ret = true;
			}
			t = time_modify;
		}
		return ret;
	}
	static thread_hot ref;
};
