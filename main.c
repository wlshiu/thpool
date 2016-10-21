#include <stdio.h>
#include <stdlib.h>
#include "thpool.h"
#include "pthread.h"
#include "windows.h"
#include "log.h"

//===============================================
//===============================================
LOG_FLAG_s          gLog_flags;
pthread_mutex_t     g_mutex_msg;
//===============================================
static void
_job_proc(
    thpool_job_t    *pJob)
{
    dbg("tid=x%08lx: %u\n", pJob->tid, pJob->tunnel_info[0].u32_value);
    return;
}

static void
_job_del(
    thpool_job_t    *pJob)
{
    dbg("tid=x%08lx: %u\n", pJob->tid, pJob->tunnel_info[0].u32_value);
    return;
}
//===============================================

int main()
{
    int                     i, job_amount = 5;
    thpool_err_t            rval = 0;
    thpool_t                *pHThpool = 0;
    thpool_init_info_t      init_info = {0};

    pthread_mutex_init(&g_mutex_msg, NULL);

    LOG_FLAG_ZERO(&gLog_flags);
    LOG_FLAG_SET(&gLog_flags, LOG_THPOOL);

    init_info.thread_num = 2;
    init_info.jobq_size  = job_amount - 1;
    thpool_create(&pHThpool, &init_info);

    for(i = 0; i < job_amount; i++)
    {
        thpool_job_t    job = {0};
        job.tunnel_info[0].u32_value = i;
        job.proc                     = _job_proc;
        job.job_delete               = _job_del;
        rval = thpool_job_push(pHThpool, &job);
        if( rval )
            dbg("%d-th job: err x%x\n", i, rval);
    }

    {
        int     wait_cnt = 0;
        while( wait_cnt++ < 5 )
            Sleep(1000);
    }

    thpool_destroy(&pHThpool, 0);

    return 0;
}
