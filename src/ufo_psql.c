#include "ufo_psql.h"

#include <stdbool.h>
#include <pthread.h> // For locks

#include "psql/psql.h"

#include "debug.h"
#include "helpers.h"

#include "../include/ufo_r/src/ufos_writeback.h"

// select name from (select row_number() over () nth, name from league) as numbered where numbered.nth = 999
// select column_name, data_type from information_schema.columns where table_name = 'league'

ufo_vector_type_t psql_type_to_vector_type(psql_column_type_t type) {
    switch (type) {
        case PSQL_COL_INT: return UFO_INT;
        case PSQL_COL_REAL: return UFO_REAL;
        case PSQL_COL_CHAR: return UFO_STR;
        case PSQL_COL_BOOL: return UFO_LGL;
        case PSQL_COL_BYTE: return UFO_RAW;
        default:
        Rf_error("Unsupported column type.");
        return 0;
    }    
}

typedef struct {
    PGconn *database;
    const char *table;
    const char *column;
} psql_t;

psql_t *psql_new(PGconn *database, const char *table, const char *column) {
    psql_t *psql = (psql_t *) malloc(sizeof(psql_t));
    psql->database = database;
    psql->table = strdup(table);
    psql->column = strdup(column);
    return psql;
}

void psql_free(void *data) {
    psql_t *psql = (psql_t *) data;
    disconnect_from_database(psql->database);
    free((void *) psql->table);
    free((void *) psql->column);
    free(psql);
}

int int_action(uintptr_t index_in_vector, int index_in_target, unsigned char *target, char *element, bool missing) {
    ((int *) target)[index_in_target] = missing ? NA_INTEGER : atoi(element);
    return 0;
}

int real_action(uintptr_t index_in_vector, int index_in_target, unsigned char *target, char *element, bool missing) {
    char *remainder;
    ((double *) target)[index_in_target] = missing ? NA_REAL : strtod(element, &remainder);
    return 0;
}

int logical_action(uintptr_t index_in_vector, int index_in_target, unsigned char *target, char *element, bool missing) {
    Rboolean value;
    if (missing) {
        value = NA_LOGICAL;
    } else if (element[0] == 't' || element[0] == 'T') {
        value = TRUE;
    } else if (element[0] == 'f' || element[0] == 'F') { /// Probably unnnecessary
        value = FALSE;
    } else {
        UFO_REPORT("Cannot parse boolean value: %s", element);
        return 2;
    }
    ((Rboolean *) target)[index_in_target] = value;
    return 0;
}

int raw_action(uintptr_t index_in_vector, int index_in_target, unsigned char *target, char *element, bool missing) {
    UFO_REPORT("raw_action unimplemented");
    return 1;  
}

int string_action(uintptr_t index_in_vector, int index_in_target, unsigned char *target, char *element, bool missing) {
    UFO_REPORT("string_action unimplemented");
    return 1;  
}

int32_t intsxp_psql_populate(void* user_data, uintptr_t start, uintptr_t end, unsigned char* target) {
    psql_t *psql = (psql_t *) user_data;
    return retrieve_from_table(psql->database, psql->table, psql->column, start, end, int_action, target);
}
int32_t lglsxp_psql_populate(void* user_data, uintptr_t start, uintptr_t end, unsigned char* target) {
    psql_t *psql = (psql_t *) user_data;
    return retrieve_from_table(psql->database, psql->table, psql->column, start, end, logical_action, target);
}
int32_t rawsxp_psql_populate(void* user_data, uintptr_t start, uintptr_t end, unsigned char* target) {
    psql_t *psql = (psql_t *) user_data;
    return retrieve_from_table(psql->database, psql->table, psql->column, start, end, raw_action, target);
}
int32_t realsxp_psql_populate(void* user_data, uintptr_t start, uintptr_t end, unsigned char* target) {
    psql_t *psql = (psql_t *) user_data;
    return retrieve_from_table(psql->database, psql->table, psql->column, start, end, real_action, target);
}
int32_t strsxp_psql_populate(void* user_data, uintptr_t start, uintptr_t end, unsigned char* target) {
    psql_t *psql = (psql_t *) user_data;
    return retrieve_from_table(psql->database, psql->table, psql->column, start, end, string_action, target);
}

void intsxp_psql_writeback(void* user_data, UfoWriteListenerEvent event) {
    psql_t *psql = (psql_t *) user_data;
    //return retrieve_from_table(psql->database, psql->table, psql->column, start, end, int_action, target);
    if (event.tag == Reset) { return; }

    uintptr_t start = event.writeback.start_idx;
    uintptr_t end = event.writeback.end_idx;
    const int *data = (const int *) event.writeback.data;

    return;
}

SEXP ufo_psql(SEXP/*STRSXP*/ db, SEXP/*STRSXP*/ table, SEXP/*STRSXP*/ column, SEXP/*LGLSXP*/ read_only, SEXP/*INTSXP*/ min_load_count) {
    // Read the arguements into practical types (with checks).
    bool read_only_value = __extract_boolean_or_die(read_only);
    int min_load_count_value = __extract_int_or_die(min_load_count);
    const char *db_value = __extract_string_or_die(db);             // eg. "host=localhost port=5432 dbname=ufo user=ufo"
    const char *table_value = __extract_string_or_die(table);       // these should be sanitized
    const char *column_value = __extract_string_or_die(column);


    // Get the data
    PGconn *database = connect_to_database(db_value);
    if (NULL == database) {
        Rf_error("Cannot connect to database with connection string \"%s\".\n", db_value);
    }

    // Get vector size from database
    size_t vector_size = 0;
    int size_query_result = retrieve_size_of_table(database, table_value, &vector_size);
    if (size_query_result != 0) {
        Rf_error("Cannot calculate vector size from table \"%s\".\n", table_value);
    }

    // Get vector type from the database
    psql_column_type_t psql_type = PSQL_COL_UNSUPPORTED;
    int type_query_result = retrieve_type_of_column(database, table_value, column_value, &psql_type);
    if (type_query_result != 0) {
        Rf_error("Cannot calculate vector type from column \"%s\" of table \"%s\".\n", column_value, table_value);
    }
    ufo_vector_type_t vector_type = psql_type_to_vector_type(psql_type);

    // Create UFO
    ufo_source_t* source = (ufo_source_t*) malloc(sizeof(ufo_source_t));

    // Basics
    source->vector_size = vector_size;
    source->vector_type = vector_type;
    source->element_size = __get_element_size(vector_type);
     
        // Behavior specification
    source->data = psql_new(database, table_value, column_value);
    source->destructor_function = psql_free;
    
    switch (vector_type) {
        // case UFO_CHAR: source->population_function = charsxp_psql_populate; break;
        case UFO_REAL: source->population_function = realsxp_psql_populate; break;
        // case UFO_CPLX: source->population_function = cplxsxp_psql_populate; break;
        case UFO_LGL:  source->population_function = lglsxp_psql_populate;  break;
        case UFO_INT:  source->population_function = intsxp_psql_populate;  break;
        case UFO_RAW:  source->population_function = rawsxp_psql_populate;  break;
        case UFO_STR:  source->population_function = strsxp_psql_populate;  break;
        // case UFO_VEC:  source->population_function = vecsxp_psql_populate;  break;    
        default: Rf_error("Unsupported UFO type: %s", type2char(vector_type));
    }

    // Chunk-related parameters
    source->read_only = read_only_value;
    source->min_load_count = __select_min_load_count(min_load_count_value, source->element_size);

    // Unused.
    source->dimensions = NULL;
    source->dimensions_length = 0;

    // Call UFO constructor and return the result
    ufo_new_t ufo_new = (ufo_new_t) R_GetCCallable("ufos", "ufo_new");
    SEXP ufo = ufo_new(source);    
    return ufo;
}