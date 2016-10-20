/**
 * Copyright (c) 2016 Wei-Lun Hsu. All Rights Reserved.
 */
/** @file platform_def.h
 *
 * @author Wei-Lun Hsu
 * @version 0.1
 * @date 2016/10/20
 * @license
 * @description
 */

#ifndef __platform_def_H_wPjxPUsW_lvvs_HFIx_sHLa_u08Tbpjl7T1D__
#define __platform_def_H_wPjxPUsW_lvvs_HFIx_sHLa_u08Tbpjl7T1D__

#ifdef __cplusplus
extern "C" {
#endif

#define         USE_POSIX


#ifndef true
#define true        1
#endif

#ifndef false
#define false       0
#endif

#ifndef MEMBER_OFFSET
    #define MEMBER_OFFSET(type, member)     (unsigned long)&(((type *)0)->member)
#endif

#ifndef STRUCTURE_POINTER
    #define STRUCTURE_POINTER(type, ptr, member)    (type*)((unsigned long)ptr - MEMBER_OFFSET(type, member))
#endif


typedef void*   (*pf_thread_entry)(void*);

#if defined(USE_POSIX)
    #include "pthread.h"
    typedef pthread_t           sys_thread_t;
#else   // #if defined(USE_POSIX)
#endif // #if defined(USE_POSIX)


typedef struct sys_thread_init
{
    sys_thread_t        tid;
    pf_thread_entry     start_routine;
    void                *pArgs;

    // optional
    char                *name;
    int                 priority;
    int                 stack_size;
    void                *pStack_addr;

} sys_thread_init_t;



#if defined(USE_POSIX)

#include "unistd.h"
#include "semaphore.h"

typedef pthread_mutex_t     sys_mutex_t;
typedef sem_t               sys_sem_t;

#define _mutex_init(pMutex)       pthread_mutex_init((pMutex), NULL)
#define _mutex_deinit(pMutex)     pthread_mutex_destroy((pMutex))
#define _mutex_lock(pMutex)       pthread_mutex_lock((pMutex))
#define _mutex_unlock(pMutex)     pthread_mutex_unlock((pMutex))

#define _sem_init(pSem, value)      sem_init(pSem, 0, value)
#define _sem_destroy(pSem)          sem_destroy(pSem)
#define _sem_wait(pSem)             sem_wait(pSem)
#define _sem_post(pSem)             sem_post(pSem)

#define _usleep(us)                 usleep(us)

#define _thread_self()              pthread_self()

#define _thread_create(pThread_init_info)                                               \
    do {pthread_attr_t      attr;                                                       \
        pthread_attr_init(&attr);                                                       \
        if( (pThread_init_info)->stack_size )                                           \
            pthread_attr_setstacksize(&attr, (pThread_init_info)->stack_size);          \
        if( (pThread_init_info)->pStack_addr )                                          \
            pthread_attr_getstackaddr(&attr, (pThread_init_info)->pStack_addr);         \
        pthread_create(&((pThread_init_info)->tid), &attr,                              \
                       (pThread_init_info)->start_routine, (pThread_init_info)->pArgs); \
        pthread_attr_destroy(&attr);                                                    \
    } while(0)


#define sys_malloc(type, len)         malloc(len)
#define sys_free(ptr)                 free(ptr)

#else  // #if defined(USE_POSIX)

#endif // #if defined(USE_POSIX)

#ifdef __cplusplus
}
#endif

#endif
