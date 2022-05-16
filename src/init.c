#include "../include/ufo_r/src/ufos.h"
#include "ufo_empty.h"
#include "ufo_operators.h"

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

    // Turn on debug mode.
    {"vectors_set_debug_mode",  (DL_FUNC) &ufo_vectors_set_debug_mode,      1},

	// Artihmetic operators result constructors.
	{"fit_result", 				(DL_FUNC) &ufo_fit_result,					3},
	{"div_result",				(DL_FUNC) &ufo_div_result,					3},
	{"mod_result",				(DL_FUNC) &ufo_mod_result,					3},
	{"rel_result",				(DL_FUNC) &ufo_rel_result,					3},
	{"log_result",				(DL_FUNC) &ufo_log_result,					3},
	{"neg_result",				(DL_FUNC) &ufo_neg_result,					2},

	// Subsetting operators.
	{"subset",					(DL_FUNC) &ufo_subset,						3},
	{"subset_assign",			(DL_FUNC) &ufo_subset_assign,				4},

    {"subscript",				(DL_FUNC) &ufo_subscript,					3},

    // Terminates the function list. Necessary.
    {NULL,							NULL,										0}
};

// Initializes the package and registers the routines with the Rdynload
// library. Name follows the pattern: R_init_<package_name> 
void attribute_visible R_init_ufooperators(DllInfo *dll) {
    R_registerRoutines(dll, NULL, CallEntries, NULL, NULL);
    R_useDynamicSymbols(dll, FALSE);
    R_forceSymbols(dll, TRUE); // causes failure to lookup the ufo_get_chunk symbol
}


