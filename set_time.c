/*
 *  Rapper_skull
 *	(c) 2022
 *
 *  License: GPLv3
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <memory.h>
#include <glob.h>
#include <stdarg.h>
#include <pthread.h>
#include <execinfo.h>
#include <time.h>
#include "common.h"
//#include "dynlib.h"
#include "hook.h"

#define LIB_VERSION "v0.1"
#define LIB_TV_MODELS "C/D/E/F/H"
#define LIB_PREFIX setTVTime
#define LIB_HOOKS LIB_PREFIX##_hooks
#define hCTX LIB_PREFIX##_hook_ctx

#include "util.h"
#include "tv_info.h"
#include "C_support_custom.h"
#include "micro_tz_db/zones.h"

union
{
  const void *procs;
  const int (*TCRRTItemDimension_destructor)(void *this);
} helper_function = {
  (const void*)"_ZN18TCRRTItemDimensionD2Ev"
};

#define PROCS_SIZE 9

typedef union
{
  const void *procs[PROCS_SIZE];
  struct
  {
    const int (*TCStreamClock_StreamClock)(int *clock, void *unknown);
    const int (*TCStreamClock_SetStreamClock)(int *clock, unsigned int time, signed int offset);
    const int (*TCTv_SetDST)(void *this, int onOff);
    const int (*TCTv_GetTimeZone_C)(void *this, int *tz_id, int *offset, int unknown);
    const int (*TCTv_GetTimeZone)(void *this, int *tz_id, int *offset); // Same as above, but for D and E series
    const int (*TCStreamClock_TCStreamClock)(int *tcStreamClock);
    const int *(*TCStreamClock_destructor)(int *tcStreamClock);
    const int (*TCStreamClock_Create)(int *tcStreamClock, int offset);
    const int *(*TCStreamClock_Destroy)(int *tcStreamClock);
  };
} samyGO_whacky_t;

samyGO_whacky_t hCTX =
{
  (const void*)"_ZN13TCStreamClock11StreamClockEPb",
  (const void*)"_ZN13TCStreamClock14SetStreamClockEmi",
  (const void*)"_ZN4TCTv6SetDSTEi",
  (const void*)"_ZN4TCTv11GetTimeZoneEPiS0_i",
  (const void*)"_ZN4TCTv11GetTimeZoneEPiS0_",
  (const void*)"_ZN13TCStreamClockC1Ev",
  (const void*)"_ZN13TCStreamClockD1Ev",
  (const void*)"_ZN13TCStreamClock6CreateEi",
  (const void*)"_ZN13TCStreamClock7DestroyEv",
};

unsigned int *TCStreamClock_TCStreamClock = NULL;
unsigned int *TCStreamClock_destructor = NULL;
unsigned int *TCStreamClock_Create = NULL;
unsigned int *TCStreamClock_Destroy = NULL;

EXTERN_C void lib_init(void *_h, const char *libpath){
	char *argv[100], *optstr, *p;
	int argc, model;
  unsigned int *addr, *ldr_addr;
  int clock[3];
  int tz_id, offset;
  const char *timezone = "UTC";
  time_t now, newtime;
  struct tm timeinfo;

  unlink(LOG_FILE);

	log("SamyGO "LIB_TV_MODELS" lib"LIB_NAME" "LIB_VERSION" - (c) Rapper_skull 2022\n");

  argc = getArgCArgV(libpath, argv);
  if(argc > 1){
    timezone = argv[1];
  }

  void *h = dlopen(0, RTLD_LAZY);
  if(!h)
  {
    char *serr = dlerror();
    log("dlopen error %s\n", serr);
    return;
  }

  patch_adbg_CheckSystem(h);

  model = getTVModel();
  logf("TV Model: %s\n", tvModelToStr(model));

  if(model == TV_MODEL_C){
    samyGO_whacky_t_init(h, &helper_function, 1); // Get function to calculate other offsets
    if(helper_function.TCRRTItemDimension_destructor){
      addr = find_function_return_from_r11((unsigned int *)helper_function.TCRRTItemDimension_destructor);
      addr = find_function_return_from_r11(addr) - 7;
      if(addr){
        ldr_addr = check_is_LDR_RD((unsigned int *)addr);
        if(ldr_addr){
          TCStreamClock_TCStreamClock = addr;
          addr = (unsigned int *)*(unsigned int *)ldr_addr;
          if(addr){
            TCStreamClock_destructor = (unsigned int *)addr[0];
            TCStreamClock_Create = (unsigned int *)addr[2];
            TCStreamClock_Destroy = (unsigned int *)addr[3];
          }
        }
      }
    }
  }

  samyGO_whacky_t_init(h, &hCTX, PROCS_SIZE);

  const char *posix_str = micro_tz_db_get_posix_str(timezone);
  if(posix_str){
    log("TZ string for timezone %s is %s\n", timezone, posix_str);
    setenv("TZ", posix_str, 1);
    tzset();
  }

  if(hCTX.TCStreamClock_TCStreamClock && hCTX.TCStreamClock_destructor && hCTX.TCStreamClock_Create && hCTX.TCStreamClock_Destroy
    && hCTX.TCStreamClock_SetStreamClock && hCTX.TCTv_SetDST && hCTX.TCStreamClock_StreamClock && ( hCTX.TCTv_GetTimeZone_C || hCTX.TCTv_GetTimeZone ) ){
    hCTX.TCStreamClock_TCStreamClock(&clock[0]);
    hCTX.TCStreamClock_Create(&clock[0], 0);
    if(hCTX.TCTv_GetTimeZone_C){
      hCTX.TCTv_GetTimeZone_C(NULL, &tz_id, &offset, 1);
    } else {
      hCTX.TCTv_GetTimeZone(NULL, &tz_id, &offset);
    }
    time(&now);
    localtime_r(&now, &timeinfo);
    newtime = now + timeinfo.tm_gmtoff - (offset * 60); // Compensates for TV with wrong TimeZone set
    if(timeinfo.tm_isdst){  // TV offset is indipendent of DST settings
      newtime -= 3600;
    }
    hCTX.TCStreamClock_SetStreamClock(&clock[0], newtime, 0);
    hCTX.TCTv_SetDST(NULL, !!timeinfo.tm_isdst);
    log("New UTC time: %d\n", hCTX.TCStreamClock_StreamClock(&clock[0], NULL));
    hCTX.TCStreamClock_Destroy(&clock[0]);
    hCTX.TCStreamClock_destructor(&clock[0]);
  }

  log("init done...\n");
  dlclose(h);
}

EXTERN_C void lib_deinit(void *_h)
{
  log(">>> %s\n", __func__);

  log("<<< %s\n", __func__);
}
