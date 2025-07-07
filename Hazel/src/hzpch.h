#pragma once

#include <iostream>
#include <memory>
#include <utility>
#include <algorithm>
#include <functional>

#include <string>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <unordered_set>

#include "Hazel/Log.h"

#ifdef HZ_PLATFORM_WINDOWS
	#define NOMINMAX
	#include <Windows.h>
	#include <commdlg.h>
	#undef NOMINMAX
#endif

