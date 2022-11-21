#pragma once

#include "leia/headTracking/common/api.h"

#define LEIAHEADTRACKING_VERSION "0.6.87"
#define LEIAHEADTRACKING_MAJOR_VERSION 0
#define LEIAHEADTRACKING_MINOR_VERSION 6
#define LEIAHEADTRACKING_PATCH_VERSION 87

BEGIN_CAPI_DECL

LHT_COMMON_API
const char* leiaHeadTrackingGetVersion();

END_CAPI_DECL
