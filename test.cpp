
//关闭非活动连接
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <assert.h>
#include <signal.h>
#include <pthread.h>

#include "TimeWheel.h"


//每tick间隔的毫秒数目 10毫秒
#define MS_PER_TICK 10
//每秒的tick个数
#define TICKS_PER_SECOND 100
 
pthread_t g_TimerThread; /* 定时器线程 */
pthread_t g_SendThread; /* 收发线程 */
TimeWheel *pst_gcTimeout; /* 时间轮指针 */
pthread_mutex_t timerMutex;/* 用来锁时间轮定时器的队列的 */

void cb_func(user_data *user_data);     //定时器回调函数


//任务延迟 tick*10 毫秒
int taskDelay(int tick)
{
	struct timeval delay;
	
	ulong ulwaitMsTime = 0;
	ulwaitMsTime = tick*MS_PER_TICK;//毫秒

	delay.tv_sec = ulwaitMsTime/1000; //延迟的秒数
	delay.tv_usec = ulwaitMsTime*1000 -  delay.tv_sec*1000000;//微秒，这个值是要减去之前的秒数的值的，得到的是剩下的微秒值，两个值的结果才是最后delay的时间

	//延迟tick指定的时间
	(void)select(0,NULL,NULL,NULL,&delay);

	return 0;
}

/* 定时器线程 */
void *Timer_TickRun(void *param)
{
	static uint ulCount = 0;
	struct timeval stOldTime,stNewTime;
	uint ulLoop;
	
	prctl(PR_SET_NAME,"Timer_TickRun");
	printf("TimeWheel: thread Timer_TickRun begin!\r\n");

	//获取当前系统时间
	gettimeofday(&stOldTime,NULL);

    while (1)
    {
		taskDelay(1);//10 ms执行一次
		pst_gcTimeout->tick(1);//时间轮往后走一格
		ulCount++;//ulcount计数

		//每秒做一次校正 100个tick
		if(ulCount >= TICKS_PER_SECOND)
		{
			gettimeofday(&stNewTime,NULL);
			//如果当前时间和上一个时间对比还小，这个说明用户可能修改了时间了，而且是修改的是比之前的事情还要早的时间
			if((stNewTime.tv_sec*1000 + stNewTime.tv_usec/1000) < (stOldTime.tv_sec*1000 + stOldTime.tv_usec/1000))
			{
				stOldTime = stNewTime;//重新计时
				ulCount = 0;//置为0，重新计数
				printf("TimeWheel: thread Timer_TickRun newtime < oldtime!\r\n");
				continue;
			}
			//当前时间和之前时间的差值是 秒 和 微秒两个组合而成
			ulLoop = (stNewTime.tv_sec*1000 + stNewTime.tv_usec/1000) - (stOldTime.tv_sec*1000 + stOldTime.tv_usec/1000);//差多少毫秒
			ulLoop /= MS_PER_TICK;//ulLoop 得到的是当前时间和之前时间的 tick的个数		

			//更新oldtime变量
			stOldTime = stNewTime;//重新计时

			//触发遗漏的tick的处理
			if(ulLoop > ulCount)
			{
				ulLoop -= ulCount;
				if(ulLoop > TICKS_PER_SECOND)
				{
					//最多补遗漏1s 也就是 TICKS_PER_SECOND
					pst_gcTimeout->tick(TICKS_PER_SECOND);//时间轮往后走100格	
				}
				else
				{
					pst_gcTimeout->tick(ulLoop);//时间轮往后走ulLoop格	
				}
			}

			ulCount = 0;//置为0，重新计数
		}
    }

	return NULL;
}


int main(int argc, char *argv[])
{
	int ret = -1;
	user_data *ptmp1;
	user_data *ptmp2;
	user_data *ptmp3;
	
	//初始化锁
    pthread_mutex_init(&timerMutex,NULL); 

	pst_gcTimeout = new TimeWheel();
	if(NULL == pst_gcTimeout)
	{
		printf("main: new TimeWheel fail! \r\n");
		return 1;
	}

    /* 2：创建底层的发送线程，接收来自app层的数据（发送的sendbuf中数据），socket用的和接收的是同一个socket */
	ret = pthread_create(&g_TimerThread, NULL, &Timer_TickRun, NULL);
	if(ret != 0)
	{
		printf("RunServer: pthread_create SendRun fail! ret=[%d]\r\n",ret);
		return 1;
	}
	
	struct tm stlocaltime;
	time_t time_seconds = time(NULL);

	localtime_r(&time_seconds,&stlocaltime);
	printf("%04d-%02d-%02d %02d:%02d:%02d :begin \r\n", 
		stlocaltime.tm_year +1900,
		stlocaltime.tm_mon + 1,
		stlocaltime.tm_mday,
		stlocaltime.tm_hour,
		stlocaltime.tm_min,
		stlocaltime.tm_sec
		);

	ptmp1 = new user_data;
	ptmp1->sockfd = 60;
	pst_gcTimeout->add_timer((ulong)60*1000, ptmp1, &cb_func);

	ptmp2 = new user_data;
	ptmp2->sockfd = 100;
	pst_gcTimeout->add_timer((ulong)100*1000, ptmp2, &cb_func);

	ptmp3 = new user_data;
	ptmp3->sockfd = 20;
	pst_gcTimeout->add_timer((ulong)20*1000, ptmp3, &cb_func);

	for(;;)
	{
		sleep(1);
	}
 
    return 0;
}
 

void cb_func(user_data *user_data)
{
	struct tm stlocaltime;
	time_t time_seconds = time(NULL);

	localtime_r(&time_seconds,&stlocaltime);
	printf("%04d-%02d-%02d %02d:%02d:%02d :", 
		stlocaltime.tm_year +1900,
		stlocaltime.tm_mon + 1,
		stlocaltime.tm_mday,
		stlocaltime.tm_hour,
		stlocaltime.tm_min,
		stlocaltime.tm_sec
		);
    printf("time out [%d] \n", user_data->sockfd);
	delete user_data;
}



