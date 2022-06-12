#ifndef WCLIENT_COMMON_H
#define WCLIENT_COMMON_H

#include "wclient.h"

struct uvr_wc {
  struct uvr_wc_surface wcsurf;
  struct uvr_wc_buffer wcbuffs;
  struct uvr_wc_core_interface wcinterfaces;
};

#endif
