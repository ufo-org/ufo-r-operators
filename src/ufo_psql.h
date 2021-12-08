#pragma once

#include "Rinternals.h"

SEXP ufo_psql(SEXP/*STRSXP*/ db, SEXP/*STRSXP*/ table, SEXP/*STRSXP*/ column, SEXP/*LGLSXP*/ read_only, SEXP/*INTSXP*/ min_load_count);
