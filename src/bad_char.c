#include "bad_char.h"

#define USE_RINTERNALS
#include <R.h>
#include <Rdefines.h>
#include <Rinternals.h>
#include <R_ext/PrtUtil.h>
#include <R_ext/Itermacros.h>

/* STOLEN GOODS */
#define intCHARSXP 73
#define CHAR_RW(x) ((char *) STDVEC_DATAPTR(x))
#define BYTES_MASK (1<<1)
#define LATIN1_MASK (1<<2)
#define UTF8_MASK (1<<3)
#define CACHED_MASK (1<<5)
#define ASCII_MASK (1<<6)
#define HASHASH_MASK 1
#define IS_BYTES(x) ((x)->sxpinfo.gp & BYTES_MASK)
#define SET_BYTES(x) (((x)->sxpinfo.gp) |= BYTES_MASK)
#define IS_LATIN1(x) ((x)->sxpinfo.gp & LATIN1_MASK)
#define SET_LATIN1(x) (((x)->sxpinfo.gp) |= LATIN1_MASK)
#define IS_ASCII(x) ((x)->sxpinfo.gp & ASCII_MASK)
#define SET_ASCII(x) (((x)->sxpinfo.gp) |= ASCII_MASK)
#define IS_UTF8(x) ((x)->sxpinfo.gp & UTF8_MASK)
#define SET_UTF8(x) (((x)->sxpinfo.gp) |= UTF8_MASK)
#define ENC_KNOWN(x) ((x)->sxpinfo.gp & (LATIN1_MASK | UTF8_MASK))
#define SET_CACHED(x) (((x)->sxpinfo.gp) |= CACHED_MASK)
#define IS_CACHED(x) (((x)->sxpinfo.gp) & CACHED_MASK)
#define ISNULL(x) ((x) == R_NilValue)

#define HASHSIZE(x)	     ((int) STDVEC_LENGTH(x))
#define HASHPRI(x)	     ((int) STDVEC_TRUELENGTH(x))
#define HASHTABLEGROWTHRATE  1.2
#define HASHMINSIZE	     29
#define SET_HASHPRI(x,v)     SET_TRUELENGTH(x,v)
#define HASHCHAIN(table, i)  ((SEXP *) STDVEC_DATAPTR(table))[i]
#define IS_HASHED(x)	     (HASHTAB(x) != R_NilValue)

static unsigned int char_hash_size = 65536;
static unsigned int char_hash_mask = 65535;

SEXP R_StringHash;  // TODO implement an arena

# define CXHEAD(x) (x)
# define CXTAIL(x) ATTRIB(x)
SEXP (SET_CXTAIL)(SEXP x, SEXP v) {
#ifdef USE_TYPE_CHECKING
    if(TYPEOF(v) != CHARSXP && TYPEOF(v) != NILSXP)
	error("value of 'SET_CXTAIL' must be a char or NULL, not a '%s'",
	      type2char(TYPEOF(v)));
#endif
    /*CHECK_OLD_TO_NEW(x, v); *//* not needed since not properly traced */
    ATTRIB(x) = v;
    return x;
}

static unsigned int char_hash(const char *s, int len)
{
    /* djb2 as from http://www.cse.yorku.ca/~oz/hash.html */
    char *p;
    int i;
    unsigned int h = 5381;
    for (p = (char *) s, i = 0; i < len; p++, i++)
	h = ((h << 5) + h) + (*p);
    return h;
}

static int R_HashSizeCheck(SEXP table)
{
    Rf_error("unimplemented");
}

static void R_StringHash_resize(unsigned int newsize) {
    Rf_error("unimplemented");
}

/* COPYCATS */
SEXP mkBadCharLenCE(const char *name, int len, cetype_t enc)
{
    SEXP cval, chain;
    unsigned int hashcode;
    int need_enc;
    Rboolean embedNul = FALSE, is_ascii = TRUE;

    switch(enc){
        case CE_NATIVE:
        case CE_UTF8:
        case CE_LATIN1:
        case CE_BYTES:
        case CE_SYMBOL:
        case CE_ANY:
            break;
        default:
            Rf_error("unknown encoding: %d", enc);    
    }
    for (int slen = 0; slen < len; slen++) {
        if ((unsigned int) name[slen] > 127) is_ascii = FALSE;
            if (!name[slen]) embedNul = TRUE;
    }
    if (embedNul) {
        /* This is tricky: we want to make a reasonable job of
        representing this string, and EncodeString() is the most
        comprehensive */
        SEXP c = allocVector(intCHARSXP, len);
        memcpy(CHAR_RW(c), name, len);

        switch(enc) {
            case CE_UTF8: SET_UTF8(c); break;
            case CE_LATIN1: SET_LATIN1(c); break;
            case CE_BYTES: SET_BYTES(c); break;
            default: break;
        }
        if (is_ascii) {
            SET_ASCII(c);
        }
        Rf_error("embedded nul in string: '%s'", CHAR(c));
                //EncodeString(c, 0, 0, Rprt_adj_none));
    }

    if (enc && is_ascii) {
        enc = CE_NATIVE;
    }
    switch(enc) {
        case CE_UTF8: need_enc = UTF8_MASK; break;
        case CE_LATIN1: need_enc = LATIN1_MASK; break;
        case CE_BYTES: need_enc = BYTES_MASK; break;
        default: need_enc = 0;
    }

    hashcode = char_hash(name, len) & char_hash_mask;

    /* Search for a cached value */
    cval = R_NilValue;
    chain = VECTOR_ELT(R_StringHash, hashcode);
    for (; !ISNULL(chain) ; chain = CXTAIL(chain)) {
	SEXP val = CXHEAD(chain);
	if (TYPEOF(val) != CHARSXP) break; /* sanity check */
	if (need_enc == (ENC_KNOWN(val) | IS_BYTES(val)) &&
	    LENGTH(val) == len &&  /* quick pretest */
	    (!len || (memcmp(CHAR(val), name, len) == 0))) { // called with len = 0
	    cval = val;
	    break;
	}
    }
    if (cval == R_NilValue) {
	/* no cached value; need to allocate one and add to the cache */
	PROTECT(cval = allocVector(intCHARSXP, len));
	memcpy(CHAR_RW(cval), name, len);
	switch(enc) {
	case CE_NATIVE:
	    break;          /* don't set encoding */
	case CE_UTF8:
	    SET_UTF8(cval);
	    break;
	case CE_LATIN1:
	    SET_LATIN1(cval);
	    break;
	case CE_BYTES:
	    SET_BYTES(cval);
	    break;
	default:
	    error("unknown encoding mask: %d", enc);
	}
	if (is_ascii) SET_ASCII(cval);
	SET_CACHED(cval);  /* Mark it */
	/* add the new value to the cache */
	chain = VECTOR_ELT(R_StringHash, hashcode);
	if (ISNULL(chain))
	    SET_HASHPRI(R_StringHash, HASHPRI(R_StringHash) + 1);
	/* this is a destrictive modification */
	chain = SET_CXTAIL(cval, chain);
	SET_VECTOR_ELT(R_StringHash, hashcode, chain);

	/* resize the hash table if necessary with the new entry still
	   protected.
	   Maximum possible power of two is 2^30 for a VECSXP.
	   FIXME: this has changed with long vectors.
	*/
	if (R_HashSizeCheck(R_StringHash)
	    && char_hash_size < 1073741824 /* 2^30 */)
	    R_StringHash_resize(char_hash_size * 2);

	UNPROTECT(1);
    }
    return cval;
}