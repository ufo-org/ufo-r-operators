#include "ufo_mutate.h"

#define USE_RINTERNALS
#include <R.h>
#include <Rinternals.h>
#include <R_ext/Itermacros.h>

#include "safety_first.h"
#include "safety_first.h"

#include "ufo_operators.h"
#include "ufo_empty.h"
#include "ufo_coerce.h"

void set_integer_by_integer_index(SEXP vector, R_xlen_t vector_length, int index, int value) {
	if (index == NA_INTEGER) {
		return;
	}
	R_xlen_t vector_index = ((R_xlen_t) index) - 1;
	make_sure(vector_index < vector_length,
	  		  "Index out of bounds %d >= %d.", vector_index, vector_length);
	safely_set_integer(vector, vector_index, value);
}

void set_integer_by_real_index(SEXP vector, R_xlen_t vector_length, R_xlen_t index, int value) {
	if (safely_is_na_xlen(index)) {
		return;
	}
	R_xlen_t vector_index = ((R_xlen_t) index) - 1;
	make_sure(vector_index < vector_length,
	  		  "Index out of bounds %d >= %d.", vector_index, vector_length); // TODO redundant
	safely_set_integer(vector, vector_index, value);
}

void set_real_by_integer_index(SEXP vector, R_xlen_t vector_length, int index, double value) {
	if (safely_is_na_integer(index)) {
		return;
	}
	R_xlen_t vector_index = ((R_xlen_t) index) - 1;
	make_sure(vector_index < vector_length, 
	  		  "Index out of bounds %d >= %d.", vector_index, vector_length); // TODO redundant
	safely_set_real(vector, vector_index, value);
}

void set_real_by_real_index(SEXP vector, R_xlen_t vector_length, R_xlen_t index, double value) {
	if (safely_is_na_xlen(index)) {
		return;
	}
	R_xlen_t vector_index = ((R_xlen_t) index) - 1;
	make_sure(vector_index < vector_length, 
	  		  "Index out of bounds %d >= %d.", vector_index, vector_length); // TODO redundant
	safely_set_real(vector, vector_index, value);
}

void set_complex_by_integer_index(SEXP vector, R_xlen_t vector_length, int index, Rcomplex value) {
	if (safely_is_na_integer(index)) {
		return;
	}
	R_xlen_t vector_index = ((R_xlen_t) index) - 1;
	make_sure(vector_index < vector_length,
	  		  "Index out of bounds %d >= %d.", vector_index, vector_length);
	safely_set_complex(vector, vector_index, value);
}

void set_complex_by_real_index(SEXP vector, R_xlen_t vector_length, R_xlen_t index, Rcomplex value) {
	if (safely_is_na_xlen(index)) {
		return;
	}
	R_xlen_t vector_index = ((R_xlen_t) index) - 1;
	make_sure(vector_index < vector_length,
	  		  "Index out of bounds %d >= %d.", vector_index, vector_length);
	safely_set_complex(vector, vector_index, value);
}

void set_logical_by_integer_index(SEXP vector, R_xlen_t vector_length, int index, Rboolean value) {
	if (safely_is_na_integer(index)) {
		return;
	}
	R_xlen_t vector_index = ((R_xlen_t) index) - 1;
	make_sure(vector_index < vector_length,
	  		  "Index out of bounds %d >= %d.", vector_index, vector_length);
	safely_set_logical(vector, vector_index, value);
}

void set_logical_by_real_index(SEXP vector, R_xlen_t vector_length, R_xlen_t index, Rboolean value) {
	if (safely_is_na_xlen(index)) {
		return;
	}
	R_xlen_t vector_index = ((R_xlen_t) index) - 1;
	make_sure(vector_index < vector_length,
	  		  "Index out of bounds %d >= %d.", vector_index, vector_length);
	safely_set_logical(vector, vector_index, value);
}

void set_raw_by_integer_index(SEXP vector, R_xlen_t vector_length, int index, Rbyte value) {
	if (safely_is_na_integer(index)) {
		return;
	}
	R_xlen_t vector_index = ((R_xlen_t) index) - 1;
	make_sure(vector_index < vector_length,
	  		  "Index out of bounds %d >= %d.", vector_index, vector_length);
	safely_set_raw(vector, vector_index, value);
}

void set_raw_by_real_index(SEXP vector, R_xlen_t vector_length, R_xlen_t index, Rbyte value) {
	if (safely_is_na_xlen(index)) {
		return;
	}
	R_xlen_t vector_index = ((R_xlen_t) index) - 1;
	make_sure(vector_index < vector_length,
	  		  "Index out of bounds %d >= %d.", vector_index, vector_length);
	safely_set_raw(vector, vector_index, value);
}


void set_string_by_integer_index(SEXP vector, R_xlen_t vector_length, int index, SEXP/*CHARSXP*/ value) {
	if (safely_is_na_integer(index)) {
		return;
	}
	R_xlen_t vector_index = ((R_xlen_t) index) - 1;
	make_sure(vector_index < vector_length,
	  		  "Index out of bounds %d >= %d.", vector_index, vector_length);
	safely_set_string(vector, vector_index, value);
}

void set_string_by_real_index(SEXP vector, R_xlen_t vector_length, R_xlen_t index, SEXP/*CHARSXP*/ value) {
	if (safely_is_na_xlen(index)) {
		return;
	}
	R_xlen_t vector_index = ((R_xlen_t) index) - 1;
	make_sure(vector_index < vector_length,
	  		  "Index out of bounds %d >= %d.", vector_index, vector_length);
	safely_set_string(vector, vector_index, value);
}

// TODO Index 
SEXP ufo_update(SEXP vector, SEXP subscript, SEXP values, SEXP min_load_count_sexp) {
	//int32_t min_load_count = (int32_t) __extract_int_or_die(min_load_count_sexp);
	SEXP indices = ufo_subscript(vector, subscript, min_load_count_sexp);
    
	switch (TYPEOF(indices)) {
	case INTSXP:
		return write_values_into_vector_at_integer_indices(vector, indices, values);
	case REALSXP:
		return write_values_into_vector_at_real_indices(vector, indices, values);
	default:
		Rf_error("Cannot subset assign: index of invalid type %s", 
		         type2char(TYPEOF(indices)));
		return R_NilValue;
	}
}

// XXX Can copy by region for contiguous, ordered areas of memory
SEXP write_values_into_vector_at_integer_indices(SEXP target, SEXP indices_into_target, SEXP source) {
	make_sure(TYPEOF(indices_into_target) == INTSXP, //Rf_error, 
	 		  "Index vector was expected to be of type INTSXP, but found %i.",
	 		  type2char(TYPEOF(indices_into_target)));

	R_xlen_t index_length = XLENGTH(indices_into_target);
	R_xlen_t source_length = XLENGTH(source);
	R_xlen_t target_length = XLENGTH(target);

	make_sure(source_length <= index_length,
			  "The source vector must be the same size or smaller than "
			  "the index vector when copying selected values "
			  "into a vector.");

	make_sure(index_length % source_length == 0,
			  "The source vector's size must be a multiple of "
			  "the index vector when copying selected values "
			  "into a vector.");

	switch (TYPEOF(target))	{
	case INTSXP:
		for (R_xlen_t i = 0; i < index_length; i++) {
			R_xlen_t index_in_source = i % source_length;
			int value = element_as_integer(source, index_in_source);
			int index_in_target = safely_get_integer(indices_into_target, i);
			set_integer_by_integer_index(target, target_length, index_in_target, value);
		}
		break;
	
	case REALSXP:
		for (R_xlen_t i = 0; i < index_length; i++) {
			R_xlen_t index_in_source = i % source_length;
			double value = element_as_real(source, index_in_source);
			int index_in_target = safely_get_integer(indices_into_target, i);
			set_real_by_integer_index(target, target_length, index_in_target, value);
		}
		break;

	case CPLXSXP:
		for (R_xlen_t i = 0; i < index_length; i++) {
			R_xlen_t index_in_source = i % source_length;
			Rcomplex value = element_as_complex(source, index_in_source);
			int index_in_target = safely_get_integer(indices_into_target, i);
			set_complex_by_integer_index(target, target_length, index_in_target, value);
		}
		break;

	case LGLSXP:
		for (R_xlen_t i = 0; i < index_length; i++) {
			R_xlen_t index_in_source = i % source_length;
			Rboolean value = element_as_logical(source, index_in_source);
			int index_in_target = safely_get_integer(indices_into_target, i);
			set_logical_by_integer_index(target, target_length, index_in_target, value);
		}
		break;

	case STRSXP:
		for (R_xlen_t i = 0; i < index_length; i++) {
			R_xlen_t index_in_source = i % source_length;
			SEXP/*STRSXP*/ value = element_as_string(source, index_in_source);
			int index_in_target = safely_get_integer(indices_into_target, i);
			set_string_by_integer_index(target, target_length, index_in_target, value);
		}
		break;

	case RAWSXP:
		for (R_xlen_t i = 0; i < index_length; i++) {
			R_xlen_t index_in_source = i % source_length;
			Rbyte value = element_as_raw(source, index_in_source);
			int index_in_target = safely_get_integer(indices_into_target, i);
			set_raw_by_integer_index(target, target_length, index_in_target, value);
		}
		break;

		// TODO potentially others: string?
	
	default:
		Rf_error("Cannot copy selected values from vector of type %i to "
		         "vector of type %i", 
		         type2char(TYPEOF(source)), type2char(TYPEOF(target)));
	}

	return target;
}

// XXX Can copy by region for contiguous, ordered areas of memory
SEXP write_values_into_vector_at_real_indices(SEXP target, SEXP indices_into_target, SEXP source) {
	make_sure(TYPEOF(indices_into_target) == INTSXP,
	 		  "Index vector was expected to be of type INTSXP, but found %i.",
	 		  type2char(TYPEOF(indices_into_target)));

	R_xlen_t index_length = XLENGTH(indices_into_target);
	R_xlen_t source_length = XLENGTH(source);
	R_xlen_t target_length = XLENGTH(target);

	make_sure(source_length <= index_length,
			  "The source vector must be the same size or smaller than "
			  "the index vector when copying selected values "
			  "into a vector.");

	make_sure(index_length % source_length == 0,
			  "The source vector's size must be a multiple of "
			  "the index vector when copying selected values "
			  "into a vector.");

	switch (TYPEOF(source))	{
	case INTSXP:
		for (R_xlen_t i = 0; i < index_length; i++) {
			R_xlen_t index_in_source = i % source_length;
			int value = element_as_integer(source, index_in_source);
			R_xlen_t index_in_target = safely_get_xlen(indices_into_target, i);
			set_integer_by_real_index(target, target_length, index_in_target, value);
		}
		break;
	
	case REALSXP:
		for (R_xlen_t i = 0; i < index_length; i++) {
			R_xlen_t index_in_source = i % source_length;
			double value = element_as_real(source, index_in_source);
			R_xlen_t index_in_target = safely_get_xlen(indices_into_target, i);
			set_real_by_real_index(target, target_length, index_in_target, value);
		}
		break;

	case CPLXSXP:
		for (R_xlen_t i = 0; i < index_length; i++) {
			R_xlen_t index_in_source = i % source_length;
			Rcomplex value = element_as_complex(source, index_in_source);
			R_xlen_t index_in_target = safely_get_xlen(indices_into_target, i);
			set_complex_by_real_index(target, target_length, index_in_target, value);
		}
		break;

	case LGLSXP:
		for (R_xlen_t i = 0; i < index_length; i++) {
			R_xlen_t index_in_source = i % source_length;
			Rboolean value = element_as_logical(source, index_in_source);
			R_xlen_t index_in_target = safely_get_xlen(indices_into_target, i);
			set_logical_by_real_index(target, target_length, index_in_target, value);
		}
		break;

	case RAWSXP:
		for (R_xlen_t i = 0; i < index_length; i++) {
			R_xlen_t index_in_source = i % source_length;
			SEXP/*STRSXP*/ value = element_as_string(source, index_in_source);
			R_xlen_t index_in_target = safely_get_xlen(indices_into_target, i);
			set_string_by_real_index(target, target_length, index_in_target, value);
		}
		break;


	case STRSXP:
		for (R_xlen_t i = 0; i < index_length; i++) {
			R_xlen_t index_in_source = i % source_length;
			Rbyte value = element_as_raw(source, index_in_source);
			R_xlen_t index_in_target = safely_get_xlen(indices_into_target, i);
			set_raw_by_real_index(target, target_length, index_in_target, value);
		}
		break;

		// TODO potentially others
	
	default:
		Rf_error("Cannot copy selected values from vector of type %i to "
		         "vector of type %i", 
		         type2char(TYPEOF(source)), type2char(TYPEOF(target)));
	}

	return target;
}
