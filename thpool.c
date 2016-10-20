/**
 * Copyright (c) 2016 Wei-Lun Hsu. All Rights Reserved.
 */
/** @file thpool.c
 *
 * @author Wei-Lun Hsu
 * @version 0.1
 * @date 2016/10/20
 * @license
 * @description
 */


#include <stdlib.h>
#include <string.h>
#include "platform_def.h"
#include "thpool.h"
#include "log.h"

#include "jobq.h"

//=============================================================================
//                  Constant Definition
//=============================================================================

//=============================================================================
//                  Macro Definition
//=============================================================================

//=============================================================================
//                  Structure Definition
//=============================================================================
typedef struct job_box
{
    struct job_box  *next;

    thpool_job_t    job;
} job_box_t;

typedef struct job_boxq
{
    sys_mutex_t     rwmutex;             // used for queue r/w access
    job_box_t       *pHead;              // pointer to front of queue
    job_box_t       *pTail;              // pointer to rear  of queue
    sys_sem_t       sem_has_jobs;        // flag as binary semaphore
    int             cur_jobs;            // number of jobs in queue

} job_boxq_t;

/**
 *  private info of a thread
 */
typedef struct thread_info
{
    long    is_exist;

    void    *pThpool_dev;

} thread_info_t;


/**
 *  thpool's device
 */
typedef struct thpool_dev
{
    int         total_threads;
    int         total_job_boxes;

    sys_sem_t   sem_job_r;
    sys_sem_t   sem_job_w;

    thpool_job_t    *pJob_pool;
    //--------------
    jobq_t      *pJobq;
    //--------------

    thread_info_t       *pThread_priv_info;

} thpool_dev_t;
//=============================================================================
//                  Global Data Definition
//=============================================================================

//=============================================================================
//                  Private Function Definition
//=============================================================================
static void*
_thread_do(void *pArgs)
{
    thread_info_t       *pThread_priv_info = (thread_info_t*)pArgs;
    thpool_dev_t        *pDev = (thpool_dev_t*)pThread_priv_info->pThpool_dev;
    jobq_t              *pJobq = 0;
    long                tid = _thread_self();

    log_err("start tid= x%lx\n", tid);
    pJobq = pDev->pJobq;

    while( pThread_priv_info->is_exist )
    {
        job_t   *pAct_job = 0;

        _sem_wait(&pDev->sem_job_r);

        _mutex_lock(&pJobq->rwmutex);
        pAct_job = _jobq_job_pop(pDev->pJobq);
        _mutex_unlock(&pJobq->rwmutex);

        if( !pAct_job )
            continue;

        pAct_job->proc(pAct_job->pArgs);

//        free_job(thpool, pAct_job);



        _sem_post(&pDev->sem_job_w);
    }

    log_err("leave tid= x%lx\n", tid);
    return NULL;
}

static thpool_err_t
_job_box_new(
    thpool_dev_t    *pDev,
    thpool_job_t    **ppJob)
{
    return 0;
}


//=============================================================================
//                  Public Function Definition
//=============================================================================
thpool_err_t
thpool_create(
    thpool_t                **ppHThpool,
    thpool_init_info_t      *pInit_info)
{
    thpool_err_t        rval = THPOOL_ERR_OK;
    thpool_dev_t        *pDev = 0;

    do{
        int     i, len = 0;

        if( *ppHThpool )
        {
            log_err("%s", "Exist thpool handle !!\n");
            rval = THPOOL_ERR_INVALID_PARAM;
            break;
        }

        if( !pInit_info )
        {
            log_err("%s", "input null pointer !\n");
            rval = THPOOL_ERR_INVALID_PARAM;
            break;
        }

        //----------------------
        // malloc device
        len = sizeof(thpool_dev_t) + pInit_info->thread_num * sizeof(thread_info_t) + 8;
        len += (pInit_info->jobq_size * sizeof(thread_info_t));
        if( !(pDev = sys_malloc(0, len)) )
        {
            rval = THPOOL_ERR_ALLOCATE_FAIL;
            break;
        }
        memset(pDev, 0x0, len);

        //----------------------
        // init
        pDev->total_threads   = pInit_info->thread_num;
        pDev->total_job_boxes = pInit_info->jobq_size;

        _sem_init(&pDev->sem_job_r, 0);
        _sem_init(&pDev->sem_job_w, pDev->total_threads);

        // XXX: job queue model can be change.
        _jobq_init(&pDev->pJobq);

        // create thread
        pDev->pThread_priv_info = (thread_info_t*)((((unsigned long)pDev + sizeof(thpool_dev_t)) + 0x7) & ~0x3);
        for(i = 0; i < pDev->total_threads; ++i)
        {
            sys_thread_init_t   init_info = {0};

            pDev->pThread_priv_info[i].is_exist    = true;
            pDev->pThread_priv_info[i].pThpool_dev = (void*)pDev;

            init_info.name          = "thread_pool";
            init_info.start_routine = _thread_do;
            init_info.pArgs         = &pDev->pThread_priv_info[i];
            init_info.priority      = pInit_info->priority;
            init_info.stack_size    = pInit_info->stack_size;

            if( pInit_info->ppStack_addr )
                init_info.pStack_addr = pInit_info->ppStack_addr[i];

            _thread_create(&init_info);
        }

        //----------------------
        (*ppHThpool) = (void*)pDev;

    }while(0);

    if( rval != THPOOL_ERR_OK )
    {
        log_err("err 0x%x !", rval);
    }

    return rval;
}

thpool_err_t
thpool_destroy(
    thpool_t    **ppHThpool,
    void        *extraData)
{
    thpool_err_t        rval = THPOOL_ERR_OK;

    do {
        int                 i;
        thpool_dev_t        *pDev = (thpool_dev_t*)(*ppHThpool);

        if( !ppHThpool || !(*ppHThpool) )
            break;

        *ppHThpool = 0;

        // TODO: do _sem_post(&pDev->sem_job_r) for leaving thread_do()
        // TODO: meutx

        // TODO: cancel thread "is_exist = false"
        for(i = 0; i < pDev->total_threads; ++i)
        {
            pDev->pThread_priv_info[i].is_exist = false;
        }

        // TODO: Job queue cleanup
        _jobq_deinit(&pDev->pJobq);

        sys_free(pDev);
    } while(0);

    return rval;
}

thpool_err_t
thpool_job_push(
    thpool_t        *pHThpool,
    thpool_job_t    *pJob)
{
    thpool_err_t        rval = THPOOL_ERR_OK;

    do {
        thpool_dev_t    *pDev = (thpool_dev_t*)pHThpool;
        jobq_t          *pJobq = 0;

        if( !pHThpool || !pJob )
        {
            log_err("input null pointer: %p, %p\n", pHThpool, pJob);
            rval = THPOOL_ERR_INVALID_PARAM;
            break;
        }

        pJobq = pDev->pJobq;

    } while(0);

    if( rval != THPOOL_ERR_OK )
    {
        log_err("err 0x%x !", rval);
    }

    return rval;
}


/*
thpool_err_t
thpool_tamplete(
    thpool_t    *pHThpool,
    void        *extraData)
{
    thpool_err_t        rval = THPOOL_ERR_OK;


    if( rval != THPOOL_ERR_OK )
    {
        log_err("err 0x%x !", rval);
    }

    return rval;
}
*/
