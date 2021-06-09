#ifndef __TIMEWHEEL_H__
#define __TIMEWHEEL_H__

#if 1
#include <pthread.h>
#include <sys/epoll.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <unistd.h>
#include <sys/time.h>
#include <netinet/tcp.h>
#include <ctype.h>
#include <sys/sysinfo.h>
#include <sys/syscall.h>
#include <netinet/if_ether.h>
#include <netinet/ip.h>
#include <linux/if_packet.h>
#include <linux/if.h>
#include <linux/sockios.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <libgen.h>
#include <dirent.h>
#include <sys/wait.h>

#include <sys/un.h>
#include <sys/fcntl.h>
#include <semaphore.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>

#include <iostream>
#include <vector>
#include <list>
#include <string>
#include <map>
#include <set>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <functional>
#include <algorithm>
#include <errno.h>
#include <pthread.h>
 #include <sys/prctl.h>
#endif
#if 0
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include <string.h>
#endif

/* 最大设置一MTU的大小，实际内容比这个小 */
#ifndef NULL
#define NULL			0L
#endif

#ifndef __INTEGER_32_BITS__
#define __INTEGER_32_BITS__
#if defined(_WIN64) || defined(WIN64) || defined(__amd64__) || \
    defined(__x86_64) || defined(__x86_64__) || defined(_M_IA64) || \
    defined(_M_AMD64)
    typedef unsigned int ISTDUINT32;
    typedef int ISTDINT32;
#elif defined(_WIN32) || defined(WIN32) || defined(__i386__) || \
    defined(__i386) || defined(_M_X86)
    typedef unsigned long ISTDUINT32;
    typedef long ISTDINT32;
#elif defined(__MACOS__)
    typedef UInt32 ISTDUINT32;
    typedef SInt32 ISTDINT32;
#elif defined(__APPLE__) && defined(__MACH__)
    #include <sys/types.h>
    typedef u_int32_t ISTDUINT32;
    typedef int32_t ISTDINT32;
#elif defined(__BEOS__)
    #include <sys/inttypes.h>
    typedef u_int32_t ISTDUINT32;
    typedef int32_t ISTDINT32;
#elif (defined(_MSC_VER) || defined(__BORLANDC__)) && (!defined(__MSDOS__))
    typedef unsigned __int32 ISTDUINT32;
    typedef __int32 ISTDINT32;
#elif defined(__GNUC__)
    #include <stdint.h>
    typedef uint32_t ISTDUINT32;
    typedef int32_t ISTDINT32;
#else
    typedef unsigned long ISTDUINT32;
    typedef long ISTDINT32;
#endif
#endif

#ifndef __IINT8_DEFINED
#define __IINT8_DEFINED
typedef char IINT8;
#endif

#ifndef __IUINT8_DEFINED
#define __IUINT8_DEFINED
typedef unsigned char IUINT8;
#endif

#ifndef __IUINT16_DEFINED
#define __IUINT16_DEFINED
typedef unsigned short IUINT16;
#endif

#ifndef __IINT16_DEFINED
#define __IINT16_DEFINED
typedef short IINT16;
#endif

#ifndef __IINT32_DEFINED
#define __IINT32_DEFINED
typedef ISTDINT32 IINT32;
#endif

#ifndef __IUINT32_DEFINED
#define __IUINT32_DEFINED
typedef ISTDUINT32 IUINT32;
#endif

#ifndef __IINT64_DEFINED
#define __IINT64_DEFINED
#if defined(_MSC_VER) || defined(__BORLANDC__)
typedef __int64 IINT64;
#else
typedef long long IINT64;
#endif
#endif

#ifndef __IUINT64_DEFINED
#define __IUINT64_DEFINED
#if defined(_MSC_VER) || defined(__BORLANDC__)
typedef unsigned __int64 IUINT64;
#else
typedef unsigned long long IUINT64;
#endif
#endif

//6000个槽位，1个格子10ms，6000个1分钟
#define TIMEWHEEL_SLOT_NUM 6000 

#if 0
struct user_data{
	int userCBindex;//代表某个用户的控制块的索引
	TwTimer *pstTimer;//指向自己的timer链表
}
#endif


//客户端数据
struct user_data
{
    sockaddr_in address;
    int sockfd;
    char buf[1024];
    //void *pstTimer;
};

typedef void (* TIMERCB_FUN)(user_data *user_data);

// 定时器类，时间轮采用双向链表
class TwTimer {
public:
    int        rotation;  // 定时器转多少圈后生效
    int        time_slotindex;  // 记录定时器属于时间轮的哪个时间槽
    user_data* pst_userData;  // 客户数据
    TwTimer*   next;  // 指向下一个定时器
    TwTimer*   pre;  // 指向上一个定时器
public:
    TwTimer( int rot, int ts ) : rotation(rot), time_slotindex(ts), next(NULL), pre(NULL) {}
    void (*cb_func)( user_data * );  // 回调函数
};

class TimeWheel {
private: 
    int wheelSize;  // 槽的数目
    int interval;  // 定时器槽之间时间间隔,单位 毫秒
    int cur_slotIndex;  // 当前槽
	TwTimer* slot[TIMEWHEEL_SLOT_NUM];  // 时间轮的槽，指向一个定时器链表，链表无序
	
public:
    TimeWheel();
    ~TimeWheel();
    TwTimer* add_timer(ulong timeout, user_data *stUserdata, TIMERCB_FUN pfnTimerCallback);  // 根据定时值创建定时器，并插入槽中
    void del_timer( TwTimer* timer );
    void tick(ulong ulTick);
};

#endif

