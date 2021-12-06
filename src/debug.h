#pragma once

#include <Rinternals.h>

SEXP/*NILSXP*/ ufo_vectors_set_debug_mode(SEXP/*LGLSXP*/ debug);
int __get_debug_mode();

#define UFO_LOG(...) if (__get_debug_mode()) { REprintf("UFO DEBUG: ", __VA_ARGS__); }
#define UFO_LOG_SHORT(...) if (__get_debug_mode()) { REprintf(__VA_ARGS__); }

#define UFO_WARN(...) REprintf("UFO UFO_WARNING: " __VA_ARGS__)
#define UFO_REPORT(...) REprintf("UFO ERROR: " __VA_ARGS__)
#define UFO_INFO(...) REprintf("UFO INFO: " __VA_ARGS__)