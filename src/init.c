#include "../include/ufos.h"
#include "ufo_empty.h"
#include "ufo_operators.h"
#include "ufo_coerce.h"
#include "ufo_mutate.h"

#include "ufo_operators_types.h"
#include "ufo_coerce_types.h"

#include <R_ext/Rdynload.h>
#include <R_ext/Visibility.h>

// List of functions provided by the package.
static const R_CallMethodDef CallEntries[] = {

    // Chunking.
    {"get_chunk",				(DL_FUNC) &ufo_get_chunk,					4},

	// Constructors for empty vectors.
	{"intsxp_empty",			(DL_FUNC) &ufo_intsxp_empty,				3},
	{"realsxp_empty",			(DL_FUNC) &ufo_realsxp_empty,				3},
	{"cplxsxp_empty",			(DL_FUNC) &ufo_cplxsxp_empty,				3},
	{"lglsxp_empty",			(DL_FUNC) &ufo_lglsxp_empty,				3},
	{"rawsxp_empty",			(DL_FUNC) &ufo_rawsxp_empty,				2},
	{"strsxp_empty",			(DL_FUNC) &ufo_strsxp_empty,				3},
	{"vecsxp_empty",			(DL_FUNC) &ufo_vecsxp_empty,				2},

	// Artihmetic operators result constructors.
	{"fit_result", 				(DL_FUNC) &ufo_fit_result,					3},
	{"div_result",				(DL_FUNC) &ufo_div_result,					3},
	{"mod_result",				(DL_FUNC) &ufo_mod_result,					3},
	{"rel_result",				(DL_FUNC) &ufo_rel_result,					3},
	{"log_result",				(DL_FUNC) &ufo_log_result,					3},
	{"neg_result",				(DL_FUNC) &ufo_neg_result,					2},

	// Subsetting operators.
	{"subset",					(DL_FUNC) &ufo_subset,						3},
	{"update",			        (DL_FUNC) &ufo_update,						4},

    {"subscript",				(DL_FUNC) &ufo_subscript,					3},

    // Terminates the function list. Necessary.
    {NULL,						NULL,										0}
};

// Initializes the package and registers the routines with the Rdynload
// library. Name follows the pattern: R_init_<package_name> 
void attribute_visible R_init_ufooperators(DllInfo *dll) {
    R_registerRoutines(dll, NULL, CallEntries, NULL, NULL);
    R_useDynamicSymbols(dll, FALSE);
    R_forceSymbols(dll, TRUE); // causes failure to lookup the ufo_get_chunk symbol

	// Export useful functions for use by other packages in C.
	R_RegisterCCallable("ufos", "get_chunk",          (DL_FUNC) &ufo_get_chunk);
	R_RegisterCCallable("ufos", "subset",             (DL_FUNC) &ufo_subset);
	R_RegisterCCallable("ufos", "update",             (DL_FUNC) &ufo_update);
	R_RegisterCCallable("ufos", "subscript",          (DL_FUNC) &ufo_subscript);
	R_RegisterCCallable("ufos", "ufo_fit_result",     (DL_FUNC) &ufo_fit_result);
	R_RegisterCCallable("ufos", "ufo_div_result",     (DL_FUNC) &ufo_div_result);
	R_RegisterCCallable("ufos", "ufo_mod_result",     (DL_FUNC) &ufo_mod_result);
	R_RegisterCCallable("ufos", "ufo_rel_result",     (DL_FUNC) &ufo_rel_result);
	R_RegisterCCallable("ufos", "ufo_log_result",     (DL_FUNC) &ufo_log_result);
	R_RegisterCCallable("ufos", "ufo_neg_result	",    (DL_FUNC) &ufo_neg_result);
	R_RegisterCCallable("ufos", "element_as_integer", (DL_FUNC) &element_as_integer);
	R_RegisterCCallable("ufos", "element_as_real",    (DL_FUNC) &element_as_real);   
	R_RegisterCCallable("ufos", "element_as_complex", (DL_FUNC) &element_as_complex);
	R_RegisterCCallable("ufos", "element_as_string",  (DL_FUNC) &element_as_string);
	R_RegisterCCallable("ufos", "element_as_logical", (DL_FUNC) &element_as_logical);
	R_RegisterCCallable("ufos", "element_as_raw",     (DL_FUNC) &element_as_raw);
	R_RegisterCCallable("ufos", "complex",            (DL_FUNC) &complex);
	R_RegisterCCallable("ufos", "integer_as_logical", (DL_FUNC) &integer_as_logical);
	R_RegisterCCallable("ufos", "real_as_logical",    (DL_FUNC) &real_as_logical);
	R_RegisterCCallable("ufos", "complex_as_logical", (DL_FUNC) &complex_as_logical);
	R_RegisterCCallable("ufos", "string_as_logical",  (DL_FUNC) &string_as_logical);
	R_RegisterCCallable("ufos", "real_as_integer",    (DL_FUNC) &real_as_integer);
	R_RegisterCCallable("ufos", "complex_as_integer", (DL_FUNC) &complex_as_integer);
	R_RegisterCCallable("ufos", "string_as_integer",  (DL_FUNC) &string_as_integer);
	R_RegisterCCallable("ufos", "logical_as_integer", (DL_FUNC) &logical_as_integer);
	R_RegisterCCallable("ufos", "raw_as_integer",     (DL_FUNC) &raw_as_integer);
	R_RegisterCCallable("ufos", "integer_as_real",    (DL_FUNC) &integer_as_real);
	R_RegisterCCallable("ufos", "complex_as_real",    (DL_FUNC) &complex_as_real);
	R_RegisterCCallable("ufos", "string_as_real",     (DL_FUNC) &string_as_real);
	R_RegisterCCallable("ufos", "logical_as_real",    (DL_FUNC) &logical_as_real);
	R_RegisterCCallable("ufos", "raw_as_real",        (DL_FUNC) &raw_as_real);
	R_RegisterCCallable("ufos", "integer_as_complex", (DL_FUNC) &integer_as_complex);
	R_RegisterCCallable("ufos", "real_as_complex",    (DL_FUNC) &real_as_complex);
	R_RegisterCCallable("ufos", "string_as_complex",  (DL_FUNC) &string_as_complex);
	R_RegisterCCallable("ufos", "logical_as_complex", (DL_FUNC) &logical_as_complex);
	R_RegisterCCallable("ufos", "raw_as_complex",     (DL_FUNC) &raw_as_complex);
	R_RegisterCCallable("ufos", "integer_as_string",  (DL_FUNC) &integer_as_string);
	R_RegisterCCallable("ufos", "real_as_string",     (DL_FUNC) &real_as_string);
	R_RegisterCCallable("ufos", "complex_as_string",  (DL_FUNC) &complex_as_string);
	R_RegisterCCallable("ufos", "logical_as_string",  (DL_FUNC) &logical_as_string);
	R_RegisterCCallable("ufos", "raw_as_string",      (DL_FUNC) &raw_as_string);
	R_RegisterCCallable("ufos", "logical_as_raw",     (DL_FUNC) &logical_as_raw); 
}


