#include <stdio.h>
#include <stdlib.h>
#include "thpool.h"

#include "windows.h"
#include "log.h"

//===============================================
//===============================================
LOG_FLAG_s    gLog_flags;
//===============================================

int main()
{
    thpool_t                *pHThpool = 0;
    thpool_init_info_t      init_info = {0};

    LOG_FLAG_ZERO(&gLog_flags);
    LOG_FLAG_SET(&gLog_flags, LOG_THPOOL);

    init_info.thread_num = 2;
    thpool_create(&pHThpool, &init_info);

    Sleep(1000);
    thpool_destroy(&pHThpool, 0);

    return 0;
}
