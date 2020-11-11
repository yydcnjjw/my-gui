#pragma once

#define MY_DEBUG

#ifndef MY_UNUSED
#ifdef __GNUC__
#define MY_UNUSED __attribute__((unused))
#else
#define MY_UNUSED
#endif
#endif
