#pragma once

template<typename tt>
struct msg_queue{
	int back;
	int front;
	int cap;
	tt **q;

	bool multi_push;			//是否多个线程push
	lock_t  push_lock;


	void init(int cap, bool multi_push){
		back = 0;
		front = 0;
		this->cap = cap;
		this->multi_push = multi_push;
		INIT_LOCK(push_lock);
		q = (tt**)malloc(sizeof(tt*) * cap);
		if(!q){
			assert(0);
			exit(0);
		}
		memset(q, 0, sizeof(tt*)*cap);
	}
	tt* pop(){
		if (front == back){
			return NULL;
		}
		tt* ret;
		ret = q[front];
		q[front] = NULL;
		front = (front + 1) % cap;
		return ret;
	}
	bool push(tt* p){
		if(!p)
		{
			assert(0);
			return false;
		}
		if(multi_push)
		{ 
			LOCK(push_lock);
			if ((back + 1) % cap == front)
			{
				UNLOCK(push_lock);
				assert(0);
				return false;
			}
			q[back] = p;
			back = (back+1)%cap;
			UNLOCK(push_lock);
		}
		else
		{
			if ((back + 1) % cap == front)
			{
				assert(0);
				return false;
			}
			q[back] = p;
			back = (back+1)%cap;
		}
		return true;
	}

	//单线程pop时才生效
	tt* getfront()
	{
		if (front == back){
			return NULL;
		}		
		return q[front];
	}
};


//一读一写内部管理内存的队列
template<typename tt>
struct message_queue{
	int back;
	int front;
	int cap;
	int max_used;
	tt *q;
	tt* getback(){
		if ((back + 1) % cap == front){
			return 0;
		}
		return q + back;
	}
	tt* getfront(){
		if (front == back){
			return 0;
		}
		return q + front;
	}
	void init(int cap){
		back = 0;
		front = 0;
		max_used = 0;
		this->cap = cap;
		q = (tt*)malloc(sizeof(tt) * cap);
		if(!q){
			assert(0);
			exit(0);
		}
	}
	bool pop(){
		if (front == back){
			assert(0);
			return false;
		}
		front = (front + 1) % cap;
		return true;
	}
	bool push(){
		if ((back + 1) % cap == front){
			assert(0);
			return false;
		}
		back = (back + 1) % cap;
		int t = (back+cap-front)%cap;
		if(max_used < t){
			max_used = t;
		}
		return true;
	}
};
