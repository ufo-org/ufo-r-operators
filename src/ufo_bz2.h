#pragma once

#include "Rinternals.h"

SEXP ufo_intsxp_bzip2 (SEXP/*STRSXP*/ path, SEXP/*LGLSXP*/ read_only, SEXP/*INTSXP*/ min_load_count);
SEXP ufo_realsxp_bzip2(SEXP/*STRSXP*/ path, SEXP/*LGLSXP*/ read_only, SEXP/*INTSXP*/ min_load_count);
SEXP ufo_rawsxp_bzip2 (SEXP/*STRSXP*/ path, SEXP/*LGLSXP*/ read_only, SEXP/*INTSXP*/ min_load_count);
SEXP ufo_cplxsxp_bzip2(SEXP/*STRSXP*/ path, SEXP/*LGLSXP*/ read_only, SEXP/*INTSXP*/ min_load_count);
SEXP ufo_lglsxp_bzip2 (SEXP/*STRSXP*/ path, SEXP/*LGLSXP*/ read_only, SEXP/*INTSXP*/ min_load_count);
SEXP ufo_vecsxp_bzip2 (SEXP/*STRSXP*/ path, SEXP/*LGLSXP*/ read_only, SEXP/*INTSXP*/ min_load_count);
SEXP ufo_charsxp_bzip2(SEXP/*STRSXP*/ path, SEXP/*LGLSXP*/ read_only, SEXP/*INTSXP*/ min_load_count); 
SEXP ufo_strsxp_bzip2 (SEXP/*STRSXP*/ path, SEXP/*LGLSXP*/ read_only, SEXP/*INTSXP*/ min_load_count); 