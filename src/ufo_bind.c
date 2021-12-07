#pragma once

#include "Rinternals.h"

typedef enum {
    BIND_NILSXP  = 0,
    BIND_RAWSXP  = 1,
    BIND_CHARSXP = 2,
    BIND_LGLSXP  = 4,
    BIND_INTSXP  = 8,
    BIND_REALSXP = 16,
    BIND_CPLXSXP = 32,
    BIND_STRSXP  = 64,
    BIND_VECSXP  = 128,
} bind_type_t;

// Unsupported:
// UFO_CHAR = CHARSXP,
// UFO_STR  = STRSXP,
// UFO_VEC  = VECSXP,

typedef union {
    bind_type_t bind_type;
    struct {
        unsigned int RAWSXP  : 1;
        unsigned int CHARSXP : 1;
        unsigned int LGLSXP  : 1;
        unsigned int INTSXP  : 1;
        unsigned int REALSXP : 1;
        unsigned int CPLXSXP : 1;
        unsigned int STRSXP  : 1;
        unsigned int VECSXP  : 1;
    } bitfields;
} bind_type_detector_t;

ufo_vector_type_t bind_type_to_ufo_type(const bind_type_detector_t *detector) {
    switch (detector->bind_type) {
        case BIND_NILSXP:  Rf_error("No vectors were provided.");
        case BIND_RAWSXP:  return UFO_RAW;
        case BIND_CHARSXP: return UFO_CHAR;
        case BIND_LGLSXP:  return UFO_LGL;
        case BIND_INTSXP:  return UFO_INT;
        case BIND_REALSXP: return UFO_REAL;
        case BIND_CPLXSXP: return UFO_CPLX;
        case BIND_STRSXP:  return UFO_STR;
        case BIND_VECSXP:  return UFO_VEC;
    }

    Rf_error("UFO bind cannot mix vector types");
    // TODO coersion types
}

void bind_type_record (bind_type_t *bind_type, SEXPTYPE type) {
    switch(type) {
        case RAWSXP:  bind_type->RAWSXP  = 1; break;
        case LGLSXP:  bind_type->LGLSXP  = 1; break;
        case INTSXP:  bind_type->INTSXP  = 1; break;
        case REALSXP: bind_type->REALSXP = 1; break;
        case CPLXSXP: bind_type->CPLXSXP = 1; break;
        case CHARSXP: bind_type->CHARSXP = 1; break;
        case STRSXP:  bind_type->STRSXP  = 1; break;
        case VECSXP:  bind_type->VECSXP  = 1; break;
        default: Rf_error("Cannot bind object of type %s.", type2char(type))
    }
}

SEXP ufo_bind (SEXP/*VECSXP*/ vectors, SEXP/*LGLSXP*/ read_only, SEXP/*INTSXP*/ min_load_count) {
    // Read the arguements into practical types (with checks).
    bool read_only_value = __extract_boolean_or_die(read_only);
    int min_load_count_value = __extract_int_or_die(min_load_count);

    if (TYPEOF(vectors) != VECSXP) {
        Rf_error("UFO bind expected VECSXP, but found %s", type2char(TYPEOF(vectors)));
        return R_NilValue;s
    }

    if (XLENGTH(vectors) == 0) {
        return R_NilValue;
    }

    bind_type_t common_type_detector = { 0 };
    for (R_xlen_t i = 0; i < XLENGTH(vectors); i++) {
        SEXP vector = VECTOR_ELT(vectors, i);
        bind_type_record(&common_type_detector, TYPEOF(vector));
    }

    ufo_vector_type_t common_type = bind_type_to_ufo_type(&common_type_detector);
    
}
