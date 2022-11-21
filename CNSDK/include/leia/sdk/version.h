#pragma once

#include "leia/sdk/api.h"

#define LEIASDK_VERSION "0.6.87"
#define LEIASDK_MAJOR_VERSION 0
#define LEIASDK_MINOR_VERSION 6
#define LEIASDK_PATCH_VERSION 87

BEGIN_CAPI_DECL

LEIASDK_API
const char* leiaSdkGetVersion();

END_CAPI_DECL
