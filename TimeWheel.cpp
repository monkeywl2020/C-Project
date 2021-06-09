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
			pstDel = pstTmp; //Ҫɾ����ָ�븳ֵ��delָ��
			pstTmp = pstTmp->next; //ָ����һ��
			delete pstDel; //ɾ��Ҫɾ��������
		}
	}
}

//��Ӷ�ʱ���������û���ָ��ͻص�����ָ��
TwTimer* TimeWheel::add_timer(ulong timeout, user_data *pstUserdata, TIMERCB_FUN pfnTimerCallback)
{
	if(timeout <0)
	{
		return NULL;
	}

	//����ʱ���Ķ���
	pthread_mutex_lock(&timerMutex);

    // ��¼���ٸ�tick�󱻴�����������С��λinterval�ļ�Ϊ1������Ϊtimeout/interval
    int ticks = 0;
    if( timeout < interval ) {
        ticks = 1;
    } else {
        ticks = timeout / interval;//ȡ��
    }

    int rotation = ticks / wheelSize;  // ��������Ȧ��
    int ts = ( cur_slotIndex + ticks % wheelSize ) % wheelSize;  // ������Ĳۣ� ����
    TwTimer* pstTimer = new TwTimer( rotation, ts );

    // �������Ϊ�գ���ŵ�ͷ��������뵽��һ��λ��
    if( !slot[ts] ) {
        slot[ts] = pstTimer;
    } else {
        pstTimer->next = slot[ts];
        slot[ts]->pre = pstTimer;
        slot[ts] = pstTimer;
    }

	//�û�����ָ��
	pstTimer->pst_userData = pstUserdata;
	pstTimer->cb_func = pfnTimerCallback;
	
	//������ʱ���Ķ���
	pthread_mutex_unlock(&timerMutex);

    return pstTimer;
}

// ɾ����ʱ��
void TimeWheel::del_timer( TwTimer* timer ) {
    if( !timer ) {
        return;
    }
	//����ʱ���Ķ���
	pthread_mutex_lock(&timerMutex);
	
    // ע������Ϊ˫���
    int ts = timer->time_slotindex;
	//����ǵ�һ���ڵ㣬��һ���ڵ�û��pre
    if( timer == slot[ts] ) {
        slot[ts] = slot[ts]->next;
        if( slot[ts] ) {
            slot[ts]->pre = NULL;
        }
    }
    //������ǵ�һ���ڵ��򽫸ýڵ�ɾ��	
	else {
        timer->pre->next = timer->next;
        if( timer->next ) {
            timer->next->pre = timer->pre;
        }
    }
    delete timer;
	
	//������ʱ���Ķ���
	pthread_mutex_unlock(&timerMutex);

}

// intervalʱ�䵽�󣬵��ñ�������ʱ������ǰ����һ���۵ļ��
void TimeWheel::tick(ulong ulTick) 
{
	ulong i =0;

	//����ʱ���Ķ���
	pthread_mutex_lock(&timerMutex);
	
	for(; i<ulTick; i++)//��ulTick ����λ
	{
	    TwTimer* tmp = slot[cur_slotIndex + i];
	    while( tmp ) 
		{
			//���Ȧ������0��ʾҪ����1Ȧ
	        if( tmp->rotation > 0 ) 
			{  // ��ʱʱ��δ��
	            tmp->rotation--;
	            tmp = tmp->next;//����˫��������һ��
	        }
			else 
			{  // ��ʱʱ���ѵ�������timer�Ļص�����
	            tmp->cb_func( tmp->pst_userData );
	            if( tmp == slot[cur_slotIndex + i] ) 
				{  // tmpλ��������
	                slot[cur_slotIndex + i] = tmp->next;//slot[cur_slotIndex] ָ����һ���ڵ�
	                if( slot[cur_slotIndex + i] ) {
	                    slot[cur_slotIndex + i]->pre = NULL;
	                }
	                delete tmp;
	                tmp = slot[cur_slotIndex + i];
	            } 
				else 
				{  // tmpλ��������
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
	    //cur_slotIndex = ( cur_slotIndex + 1 ) % N;//cur_slotIndex �� 1 ǰ��
	}

	//������ʱ���Ķ���
	pthread_mutex_unlock(&timerMutex);

	cur_slotIndex = (cur_slotIndex + ulTick)% wheelSize;//cur_slotIndex ǰ�� ulTick ��λ�� 
}

