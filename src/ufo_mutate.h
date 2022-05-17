#pragma once

#define USE_RINTERNALS
#include <R.h>
#include <Rinternals.h>


SEXP ufo_update(SEXP vector, SEXP subscript, SEXP values, SEXP min_load_count_sexp);

SEXP write_values_into_vector_at_integer_indices(SEXP target, SEXP indices_into_target, SEXP source);

SEXP write_values_into_vector_at_real_indices(SEXP target, SEXP indices_into_target, SEXP source);