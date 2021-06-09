#include "TimeWheel.h"

extern pthread_mutex_t timerMutex;

TimeWheel::TimeWheel():wheelSize(TIMEWHEEL_SLOT_NUM),interval(10),cur_slotIndex(0){
	int i = 0;
	for(; i<TIMEWHEEL_SLOT_NUM; i++)
	{
		slot[i] = NULL;
	}
}

TimeWheel::~TimeWheel(){
	int i = 0;
	TwTimer *pstTmp = NULL;
	TwTimer *pstDel = NULL;
	for(; i<TIMEWHEEL_SLOT_NUM; i++)
	{
		pstTmp = slot[i];
		while(pstTmp != NULL)
		{
			pstDel = pstTmp; //要删除的指针赋值给del指针
			pstTmp = pstTmp->next; //指向下一个
			delete pstDel; //删除要删除的内容
		}
	}
}

//添加定时器，传入用户的指针和回调函数指针
TwTimer* TimeWheel::add_timer(ulong timeout, user_data *pstUserdata, TIMERCB_FUN pfnTimerCallback)
{
	if(timeout <0)
	{
		return NULL;
	}

	//锁定时器的队列
	pthread_mutex_lock(&timerMutex);

    // 记录多少个tick后被触发，不足最小单位interval的记为1，其余为timeout/interval
    int ticks = 0;
    if( timeout < interval ) {
        ticks = 1;
    } else {
        ticks = timeout / interval;//取整
    }

    int rotation = ticks / wheelSize;  // 被触发的圈数
    int ts = ( cur_slotIndex + ticks % wheelSize ) % wheelSize;  // 被插入的槽， 求余
    TwTimer* pstTimer = new TwTimer( rotation, ts );

    // 如果链表为空，则放到头，否则插入到第一个位置
    if( !slot[ts] ) {
        slot[ts] = pstTimer;
    } else {
        pstTimer->next = slot[ts];
        slot[ts]->pre = pstTimer;
        slot[ts] = pstTimer;
    }

	//用户数据指针
	pstTimer->pst_userData = pstUserdata;
	pstTimer->cb_func = pfnTimerCallback;
	
	//解锁定时器的队列
	pthread_mutex_unlock(&timerMutex);

    return pstTimer;
}

// 删除定时器
void TimeWheel::del_timer( TwTimer* timer ) {
    if( !timer ) {
        return;
    }
	//锁定时器的队列
	pthread_mutex_lock(&timerMutex);
	
    // 注意链表为双向的
    int ts = timer->time_slotindex;
	//如果是第一个节点，第一个节点没有pre
    if( timer == slot[ts] ) {
        slot[ts] = slot[ts]->next;
        if( slot[ts] ) {
            slot[ts]->pre = NULL;
        }
    }
    //如果不是第一个节点则将该节点删除	
	else {
        timer->pre->next = timer->next;
        if( timer->next ) {
            timer->next->pre = timer->pre;
        }
    }
    delete timer;
	
	//解锁定时器的队列
	pthread_mutex_unlock(&timerMutex);

}

// interval时间到后，调用本函数，时间轮向前滚动一个槽的间隔
void TimeWheel::tick(ulong ulTick) 
{
	ulong i =0;

	//锁定时器的队列
	pthread_mutex_lock(&timerMutex);
	
	for(; i<ulTick; i++)//走ulTick 个槽位
	{
	    TwTimer* tmp = slot[cur_slotIndex + i];
	    while( tmp ) 
		{
			//如果圈数大于0表示要减少1圈
	        if( tmp->rotation > 0 ) 
			{  // 定时时间未到
	            tmp->rotation--;
	            tmp = tmp->next;//处理双向链表下一个
	        }
			else 
			{  // 定时时间已到，调用timer的回调函数
	            tmp->cb_func( tmp->pst_userData );
	            if( tmp == slot[cur_slotIndex + i] ) 
				{  // tmp位于链表首
	                slot[cur_slotIndex + i] = tmp->next;//slot[cur_slotIndex] 指向下一个节点
	                if( slot[cur_slotIndex + i] ) {
	                    slot[cur_slotIndex + i]->pre = NULL;
	                }
	                delete tmp;
	                tmp = slot[cur_slotIndex + i];
	            } 
				else 
				{  // tmp位于链表中
	                tmp->pre->next = tmp->next;
	                if( tmp->next ) {
	                    tmp->next->pre = tmp->pre;
	                }
	                TwTimer* tmp2 = tmp->next;
	                delete tmp;
	                tmp = tmp2;
	            }
	        }
	    }
	    //cur_slotIndex = ( cur_slotIndex + 1 ) % N;//cur_slotIndex 加 1 前进
	}

	//解锁定时器的队列
	pthread_mutex_unlock(&timerMutex);

	cur_slotIndex = (cur_slotIndex + ulTick)% wheelSize;//cur_slotIndex 前进 ulTick 个位置 
}

