#pragma once

#define USE_RINTERNALS
#include <R.h>
#include <Rinternals.h>

typedef SEXP (*ufo_fit_result_t)   (SEXP x, SEXP y, SEXP/*INTSXP*/ min_load_count);
typedef SEXP (*ufo_div_result_t)   (SEXP x, SEXP y, SEXP/*INTSXP*/ min_load_count);
typedef SEXP (*ufo_mod_result_t)   (SEXP x, SEXP y, SEXP/*INTSXP*/ min_load_count);
typedef SEXP (*ufo_rel_result_t)   (SEXP x, SEXP y, SEXP/*INTSXP*/ min_load_count);
typedef SEXP (*ufo_log_result_t)   (SEXP x, SEXP y, SEXP/*INTSXP*/ min_load_count);
typedef SEXP (*ufo_neg_result_t)   (SEXP x,         SEXP/*INTSXP*/ min_load_count);

typedef SEXP (*ufo_subset_t)	   (SEXP x, SEXP y,         SEXP/*INTSXP*/ min_load_count);
typedef SEXP (*ufo_subset_assign_t)(SEXP x, SEXP y, SEXP z, SEXP/*INTSXP*/ min_load_count);

typedef SEXP (*ufo_subscript_t)    (SEXP vector, SEXP subscript, SEXP min_load_count);

typedef SEXP (*ufo_get_chunk_t)    (SEXP x, SEXP chunk, SEXP chunk_size, SEXP result_length);
