/**
 * Copyright (c) 2016 Wei-Lun Hsu. All Rights Reserved.
 */
/** @file thpool.h
 *
 * @author Wei-Lun Hsu
 * @version 0.1
 * @date 2016/10/20
 * @license
 * @description
 */

#ifndef __thpool_H_w0TnEOQO_l5Qi_HzQC_sOLF_uFJxPkAxvBcY__
#define __thpool_H_w0TnEOQO_l5Qi_HzQC_sOLF_uFJxPkAxvBcY__

#ifdef __cplusplus
extern "C" {
#endif


//=============================================================================
//                  Constant Definition
//=============================================================================
typedef enum thpool_err
{
    THPOOL_ERR_OK               = 0,
    THPOOL_ERR_ALLOCATE_FAIL,
    THPOOL_ERR_INVALID_PARAM,
    THPOOL_ERR_QUEUE_FULL,
    THPOOL_ERR_UNKNOWN
} thpool_err_t;
//=============================================================================
//                  Macro Definition
//=============================================================================

//=============================================================================
//                  Structure Definition
//=============================================================================
/**
 *  job box
 */
typedef struct thpool_job
{
    void    (*proc)(struct thpool_job *pJob);

    union {
        unsigned int        u32_value;
        unsigned long long  u64_value;
        void                *pAddr;
    } tunnel_info[2];

    void    (*job_delete)(struct thpool_job *pJob);

    // debug
    unsigned long   tid;
} thpool_job_t;

/**
 *  init info
 */
typedef struct thpool_init_info
{
    int         thread_num;     // amount of threads
    int         jobq_size;      // max jobs in queue

    // optional
    int         priority;
    int         stack_size;         // every thread has the same stack size.
    void        **ppStack_addr;     // external stack buffer

} thpool_init_info_t;

/**
 *  handle of thread pool
 */
typedef void*   thpool_t;
//=============================================================================
//                  Global Data Definition
//=============================================================================

//=============================================================================
//                  Private Function Definition
//=============================================================================

//=============================================================================
//                  Public Function Definition
//=============================================================================
thpool_err_t
thpool_create(
    thpool_t                **ppHThpool,
    thpool_init_info_t      *pInit_info);


thpool_err_t
thpool_destroy(
    thpool_t    **ppHThpool,
    void        *extraData);


thpool_err_t
thpool_job_push(
    thpool_t        *pHThpool,
    thpool_job_t    *pJob);



#ifdef __cplusplus
}
#endif

#endif
