#pragma once
template<typename tt>
struct msg_stack_pool {
	tt* list;
	tt** free_list;
	int cap;
	int top;
	int min_top;
	lock_t  lock;
	void init(int cap)
	{
		this->cap = cap;
		list = (tt*)malloc(sizeof(tt)*cap);
		if(!list)
		{
			assert(0);
			exit(0);
		}
		free_list = (tt**)malloc(sizeof(tt*)*cap);
		if(!free_list)
		{
			assert(0);
			exit(0);
		}

		for(int i= 0; i<cap; i++)
		{
			free_list[i] = &list[i];
		}
		this->top = cap;
		this->min_top = cap;
		INIT_LOCK(lock);
	}
	tt* alloc()
	{		
		LOCK(lock);
		if(top == 0)
		{
			UNLOCK(lock);
			return NULL;
		}
		tt* r = free_list[--top];
		if(top < min_top ){
			min_top = top;
		}
		UNLOCK(lock);
		return r;
	}
	bool free(tt* p)
	{
		LOCK(lock);
		if( top == cap)
		{
			UNLOCK(lock);
			assert(0);
			return false;
		}
		free_list[top++] = p;
		UNLOCK(lock);
		return true;
	}

};