#pragma once

#if defined(_WIN32)
#if defined(LIBGOSSIP_BUILD)
#define LIBGOSSIP_API __declspec(dllexport)
#else
#define LIBGOSSIP_API __declspec(dllimport)
#endif
#else
#define LIBGOSSIP_API
#endif
