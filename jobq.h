/**
 * Copyright (c) 2016 Wei-Lun Hsu. All Rights Reserved.
 */
/** @file jobq.h
 *
 * @author Wei-Lun Hsu
 * @version 0.1
 * @date 2016/10/20
 * @license
 * @description
 */

#ifndef __jobq_H_woaPeRHa_lC3D_HAsC_sO2w_unatJyndCt2Y__
#define __jobq_H_woaPeRHa_lC3D_HAsC_sO2w_unatJyndCt2Y__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <string.h>
#include "platform_def.h"
//=============================================================================
//                  Constant Definition
//=============================================================================
#define JOB_POOL_SIZE           32

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
typedef struct job
{
    struct job  *next;                  // pointer to previous job
    void        (*proc)(void* arg);     // function pointer
    void        *pArgs;                 // function's argument
} job_t;

typedef struct jobq
{
    sys_mutex_t     rwmutex;             // used for queue r/w access
    job_t           *pHead;              // pointer to front of queue
    job_t           *pTail;              // pointer to rear  of queue
    sys_sem_t       sem_has_jobs;        // flag as binary semaphore
    int             cur_jobs;            // number of jobs in queue
} jobq_t;
//=============================================================================
//                  Global Data Definition
//=============================================================================

//=============================================================================
//                  Private Function Definition
//=============================================================================
static inline int
_jobq_init(jobq_t **ppJobq)
{
    int         rval = 0;
    do {
        jobq_t      *pJobq = 0;

        if( !ppJobq )
        {
            rval = -1;
            break;
        }

        if( !(pJobq = sys_malloc(0, sizeof(jobq_t))) )
        {
            rval = -2;
            break;
        }
        memset(pJobq, 0x0, sizeof(jobq_t));

        _sem_init(&pJobq->sem_has_jobs, 0);
        _mutex_init(&pJobq->rwmutex);

        *ppJobq = pJobq;
    } while(0);
    return rval;
}

static inline int
_jobq_deinit(jobq_t **ppJobq)
{
    int         rval = 0;
    do {
        jobq_t      *pJobq = 0;

        if( !ppJobq )       break;

        pJobq = *ppJobq;
        *ppJobq = 0;

        // TODO: mutex handle
        _sem_destroy(&pJobq->sem_has_jobs);
        _mutex_deinit(&pJobq->rwmutex);

        sys_free(pJobq);

    } while(0);
    return rval;
}

static inline int
_jobq_job_push()
{
    return 0;
}

static inline job_t*
_jobq_job_pop()
{
    return 0;
}

static inline int
_jobq_job_release()
{
    return 0;
}
//=============================================================================
//                  Public Function Definition
//=============================================================================

#ifdef __cplusplus
}
#endif

#endif
