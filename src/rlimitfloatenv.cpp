#include <fenv.h>
#include <sys/time.h>
#include <sys/resource.h>
#include "../jalib/jassert.h"
#include "shareddata.h"
#include "util.h"
#include "config.h"
#include "dmtcp.h"

/*************************************************************************
 *
 *  Save and restore rlimit and float exception settings.
 *
 *************************************************************************/

namespace dmtcp
{

static int roundingMode = -1;
static fenv_t envp;
static rlim_t rlim_cur_nofile = 0;
static rlim_t rlim_cur_stack = 0;

static void
save_rlimit_float_settings()
{
  roundingMode = fegetround();
  fegetenv(&envp);

  struct rlimit rlim = {0, 0};
  getrlimit(RLIMIT_NOFILE, &rlim);
  rlim_cur_nofile = rlim.rlim_cur;

  getrlimit(RLIMIT_STACK, &rlim);
  rlim_cur_stack = rlim.rlim_cur;
}

static void
restore_rlimit_float_settings()
{
  fesetenv(&envp);
  fesetround(roundingMode);

  struct rlimit rlim = {0, 0};
  getrlimit(RLIMIT_NOFILE, &rlim);
  JWARNING(rlim_cur_nofile <= rlim.rlim_max)
    (rlim_cur_nofile) (rlim.rlim_max)
    .Text("Previous soft limit of RLIMIT_NOFILE lowered to new hard limit");
  rlim.rlim_cur = MIN(rlim_cur_nofile, rlim.rlim_max);
  JASSERT(setrlimit(RLIMIT_NOFILE, &rlim) == 0);

  getrlimit(RLIMIT_STACK, &rlim);
  JWARNING(rlim_cur_stack <= rlim.rlim_max)
    (rlim_cur_stack) (rlim.rlim_max)
    .Text("Previous soft limit of RLIMIT_STACK lowered to new hard limit");
  rlim.rlim_cur = MIN(rlim_cur_stack, rlim.rlim_max);
  JASSERT(setrlimit(RLIMIT_STACK, &rlim) == 0);
}

static void
rlimitfloat_EventHook(DmtcpEvent_t event, DmtcpEventData_t *data)
{
  switch (event) {
  case DMTCP_EVENT_PRECHECKPOINT:
    save_rlimit_float_settings();
    break;

  case DMTCP_EVENT_RESTART:
    restore_rlimit_float_settings();
    break;

  default:
    break;
  }
}

static DmtcpPluginDescriptor_t rlimitFloatPlugin = {
  DMTCP_PLUGIN_API_VERSION,
  PACKAGE_VERSION,
  "rlimit_float",
  "DMTCP",
  "dmtcp@ccs.neu.edu",
  "Rlimit/floating point plugin",
  rlimitfloat_EventHook
};


DmtcpPluginDescriptor_t
dmtcp_Rlimit_Float_PluginDescr()
{
  return rlimitFloatPlugin;
}
}
