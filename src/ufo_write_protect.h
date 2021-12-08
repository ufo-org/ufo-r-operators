#pragma once

#include "Rinternals.h"

SEXP ufo_write_protect(SEXP vector, SEXP/*LGLSXP*/ read_only, SEXP/*INTSXP*/ min_load_count);