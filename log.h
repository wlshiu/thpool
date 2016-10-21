/**
 * Copyright (c) 2016 Wei-Lun Hsu. All Rights Reserved.
 */
/** @file log.h
 *
 * @author Wei-Lun Hsu
 * @version 0.1
 * @date 2016/09/11
 * @license
 * @description
 */

#ifndef __log_H_w3ULeQks_lqbb_Hsq8_sHrj_u61es21F19Zt__
#define __log_H_w3ULeQks_lqbb_Hsq8_sHrj_u61es21F19Zt__

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>
#include <stdio.h>
//=============================================================================
//                  Constant Definition
//=============================================================================
#define MAX_LOG_FLAG_NUM            64

/**
 *  log type and it MUST be less than MAX_LOG_FLAG_NUM
 */
typedef enum LOG_TYPE
{
    LOG_BASE            = 0,
    LOG_PMSGQ,
    LOG_THPOOL,
    LOG_MISC,
    LOG_TEST,

} LOG_TYPE_e;
//=============================================================================
//                  Macro Definition
//=============================================================================
#define LOG_FLAG_SET(pLogFlag, bit_order)       ((pLogFlag)->BitFlag[(bit_order)>>5] |=  (1<<((bit_order)&0x1F)))
#define LOG_FLAG_CLR(pLogFlag, bit_order)       ((pLogFlag)->BitFlag[(bit_order)>>5] &= ~(1<<((bit_order)&0x1F)))
#define LOG_FLAG_IS_SET(pLogFlag, bit_order)    ((pLogFlag)->BitFlag[(bit_order)>>5] &   (1<<((bit_order)&0x1F)))
#define LOG_FLAG_ZERO(pLogFlag)                 memset((void*)(pLogFlag),0x0,sizeof(LOG_FLAG_s))

//=============================================================================
//                  Structure Definition
//=============================================================================
typedef struct LOG_FLAG
{
    unsigned int        BitFlag[((MAX_LOG_FLAG_NUM)+0x1F)>>5];
} LOG_FLAG_s;
//=============================================================================
//                  Global Data Definition
//=============================================================================
extern LOG_FLAG_s    gLog_flags;
//=============================================================================
//                  Private Function Definition
//=============================================================================

//=============================================================================
//                  Public Function Definition
//=============================================================================
#define log_err(str, args...)               fprintf(stderr, "%s[#%u] " str, __func__, __LINE__, ## args)
#define log_info(str, args...)              fprintf(stderr, str, ## args)

#define log(type, str, args...)             ((void)((LOG_FLAG_IS_SET(&gLog_flags, type)) ? fprintf(stderr, str, ## args): 0))
#define log_on(type)                        LOG_FLAG_SET(&gLog_flags, type)
#define log_off(type)                       LOG_FLAG_CLR(&gLog_flags, type)


#define dbg(str, args...)                   do{ extern pthread_mutex_t  g_mutex_msg;                           \
                                                pthread_mutex_lock(&g_mutex_msg);                              \
                                                fprintf(stderr, "%s[%d]=> " str, __func__, __LINE__, ## args); \
                                                pthread_mutex_unlock(&g_mutex_msg);                            \
                                            }while(0)

#undef log_err
#define log_err     dbg


#ifdef __cplusplus
}
#endif

#endif
