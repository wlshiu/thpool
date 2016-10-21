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

#include "errno.h"
//=============================================================================
//                  Constant Definition
//=============================================================================
#define ENABLE_BRAKE_JOB_PUSH       0

typedef enum job_state
{
    JOB_STATE_IDLE      = 0,
    JOB_STATE_USED,
} job_state_t;
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

    job_state_t     state;

} job_box_t;

typedef struct job_boxq
{
    sys_mutex_t     rwmutex;             // used for queue r/w access
    job_box_t       *pHead;              // pointer to front of queue
    job_box_t       *pTail;              // pointer to rear  of queue
    sys_sem_t       sem_has_jobs;        // flag as binary semaphore
    int             job_cnt;             // number of jobs in queue
    int             sequence_num;

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

    job_boxq_t          job_boxq;
    job_box_t           *pJob_box_pool;

    thread_info_t       *pThread_priv_info;

} thpool_dev_t;
//=============================================================================
//                  Global Data Definition
//=============================================================================

//=============================================================================
//                  Private Function Definition
//=============================================================================
static thpool_err_t
_job_box_create(
    thpool_dev_t    *pDev,
    thpool_job_t    *pJob)
{
    thpool_err_t    rval = THPOOL_ERR_OK;
    job_boxq_t      *pJob_boxq = &pDev->job_boxq;

    _mutex_lock(&pJob_boxq->rwmutex);
    do {
        int             i, sequence_num = 0, total_job_boxes = 0;
        job_box_t       *pJob_box_pool = 0, *pJob_box_cur = 0;

        sequence_num    = pJob_boxq->sequence_num;
        total_job_boxes = pDev->total_job_boxes;

        // get an empty job_box
        if( pJob_boxq->job_cnt >= total_job_boxes )
        {
            log_err("%s", "job queue full !\n");
            rval = THPOOL_ERR_QUEUE_FULL;
            break;
        }

        sequence_num = (sequence_num >= total_job_boxes) ? 0 : sequence_num;

        pJob_box_pool = pDev->pJob_box_pool;

        i = 0;
        while( i++ < total_job_boxes )
        {
            pJob_box_cur = pJob_box_pool + sequence_num;
            if( pJob_box_cur->state == JOB_STATE_IDLE )
                break;

            sequence_num++;
            sequence_num = (sequence_num == total_job_boxes) ? 0 : sequence_num;
        }

        pJob_boxq->sequence_num = sequence_num + 1;

        pJob_box_cur = (i < total_job_boxes) ? pJob_box_cur : 0;
        if( !pJob_box_cur )
        {
            log_err("%s", "job queue full !\n");
            rval = THPOOL_ERR_QUEUE_FULL;
            break;
        }

        // -----------------------
        // assign job info
        pJob_box_cur->next  = 0;
        pJob_box_cur->job   = *pJob;
        pJob_box_cur->state = JOB_STATE_USED;

        //-----------------------
        // update info to boxq
        if( pJob_boxq->pHead )
        {
            pJob_boxq->pTail->next = pJob_box_cur;
            pJob_boxq->pTail       = pJob_box_cur;
        }
        else
        {
            pJob_boxq->pHead = pJob_box_cur;
            pJob_boxq->pTail = pJob_box_cur;
        }

        pJob_boxq->job_cnt++;

    } while(0);

    _mutex_unlock(&pJob_boxq->rwmutex);

    return rval;
}

static thpool_err_t
_job_box_destroy(
    thpool_dev_t    *pDev,
    job_box_t       **ppJob_box)
{
    thpool_err_t    rval = THPOOL_ERR_OK;
    job_boxq_t      *pJob_boxq = &pDev->job_boxq;

    _mutex_lock(&pJob_boxq->rwmutex);
    do {
        job_box_t           *pJob_box = 0;

        if( !ppJob_box || !(*ppJob_box) )
            break;

        pJob_box = *ppJob_box;

        memset(pJob_box, 0x0, sizeof(job_box_t));

        pJob_box->state = JOB_STATE_IDLE;

        *ppJob_box = 0;
    } while(0);

    _mutex_unlock(&pJob_boxq->rwmutex);

    return rval;
}

static job_box_t*
_job_box_pop(
    thpool_dev_t    *pDev)
{
    job_boxq_t      *pJob_boxq = &pDev->job_boxq;
    job_box_t       *pJob_box_act = 0;

    _mutex_lock(&pJob_boxq->rwmutex);
    do {
        pJob_box_act = pJob_boxq->pHead;

        pJob_boxq->job_cnt--;

        if( pJob_boxq->pHead )
        {
            pJob_boxq->pHead = pJob_boxq->pHead->next;
        }
        else
        {
            pJob_boxq->pTail   = 0;
            pJob_boxq->job_cnt = 0;
        }

    } while(0);

    _mutex_unlock(&pJob_boxq->rwmutex);

    return pJob_box_act;
}


static void*
_thread_do(void *pArgs)
{
    thread_info_t       *pThread_priv_info = (thread_info_t*)pArgs;
    thpool_dev_t        *pDev = (thpool_dev_t*)pThread_priv_info->pThpool_dev;
    unsigned long       tid = _thread_self();

    dbg("start tid= x%lx\n", tid);

    while( pThread_priv_info->is_exist )
    {
        job_box_t   *pAct_job_box = 0;

        _sem_wait(&pDev->sem_job_r);

        if( !(pAct_job_box = _job_box_pop(pDev)) )
            continue;

        pAct_job_box->job.tid = tid;

        if( pAct_job_box->job.proc )
            pAct_job_box->job.proc(&pAct_job_box->job);

        if( pAct_job_box->job.job_delete )
            pAct_job_box->job.job_delete(&pAct_job_box->job);

        _job_box_destroy(pDev, &pAct_job_box);

        #if (ENABLE_BRAKE_JOB_PUSH)
        _sem_post(&pDev->sem_job_w);
        #endif
    }

    dbg("leave tid= x%lx\n", tid);
    return NULL;
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
        len += (pInit_info->jobq_size * sizeof(job_box_t) + 8);
        if( !(pDev = sys_malloc(0, len)) )
        {
            log_err("malloc fail (size= %d)\n", len);
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

        // TODO: job queue model initialize

        // create thread
        pDev->pThread_priv_info = (thread_info_t*)((((unsigned long)pDev + sizeof(thpool_dev_t)) + 0x7) & ~0x7);
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

        // job_box
        _sem_init(&pDev->job_boxq.sem_has_jobs, 0);
        _mutex_init(&pDev->job_boxq.rwmutex);

        pDev->pJob_box_pool = (job_box_t*)(((unsigned long)&pDev->pThread_priv_info[pDev->total_threads] + 0x7) & ~0x7);

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

        if( !pHThpool || !pJob )
        {
            log_err("input null pointer: %p, %p\n", pHThpool, pJob);
            rval = THPOOL_ERR_INVALID_PARAM;
            break;
        }

        #if (ENABLE_BRAKE_JOB_PUSH)
        _sem_wait(&pDev->sem_job_w);
        #endif

        if( (rval = _job_box_create(pDev, pJob)) )
           break;

        _sem_post(&pDev->sem_job_r);

    } while(0);

    if( rval != THPOOL_ERR_OK )
    {
        log_err("err 0x%x !\n", rval);
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
        log_err("err 0x%x !\n", rval);
    }

    return rval;
}
*/
