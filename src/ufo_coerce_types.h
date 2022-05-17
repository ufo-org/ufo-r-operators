#pragma once

#define USE_RINTERNALS
#include <R.h>
#include <Rinternals.h>

typedef int             (*element_as_integer_t) (SEXP source, R_xlen_t index);
typedef double          (*element_as_real_t)    (SEXP source, R_xlen_t index);
typedef Rcomplex        (*element_as_complex_t) (SEXP source, R_xlen_t index);
typedef SEXP/*CHARSXP*/ (*element_as_string_t)  (SEXP source, R_xlen_t index);
typedef Rboolean        (*element_as_logical_t) (SEXP source, R_xlen_t index);
typedef Rbyte           (*element_as_raw_t)     (SEXP source, R_xlen_t index); // Doesn't really work.

typedef Rcomplex        (*complex_t)             (double real, double imaginary);

typedef Rboolean        (*integer_as_logical_t) (int value);
typedef Rboolean        (*real_as_logical_t)    (double value);
typedef Rboolean        (*complex_as_logical_t) (Rcomplex value);
typedef Rboolean        (*string_as_logical_t)  (SEXP value);

typedef int             (*real_as_integer_t)    (double value);
typedef int             (*complex_as_integer_t) (Rcomplex value);		
typedef int             (*string_as_integer_t)  (SEXP/*CHARSXP*/ value);
typedef int             (*logical_as_integer_t) (Rboolean value);
typedef int             (*raw_as_integer_t)     (Rbyte value);

typedef double          (*integer_as_real_t)    (int value);	
typedef double          (*complex_as_real_t)    (Rcomplex value);		
typedef double          (*string_as_real_t)     (SEXP/*CHARSXP*/ value);
typedef double          (*logical_as_real_t)    (Rboolean value);
typedef double          (*raw_as_real_t)        (Rbyte value);

typedef Rcomplex        (*integer_as_complex_t) (int value);
typedef Rcomplex        (*real_as_complex_t)    (double value);
typedef Rcomplex        (*string_as_complex_t)  (SEXP/*CHARSXP*/ value);
typedef Rcomplex        (*logical_as_complex_t) (Rboolean value);
typedef Rcomplex        (*raw_as_complex_t)     (Rbyte value);

typedef SEXP/*CHARSXP*/ (*integer_as_string_t)  (int value);
typedef SEXP/*CHARSXP*/ (*real_as_string_t)     (double value);
typedef SEXP/*CHARSXP*/ (*complex_as_string_t)  (Rcomplex value);		
typedef SEXP/*CHARSXP*/ (*logical_as_string_t)  (Rboolean value);
typedef SEXP/*CHARSXP*/ (*raw_as_string_t)      (Rbyte value);

typedef Rbyte           (*logical_as_raw_t)     (Rboolean value);