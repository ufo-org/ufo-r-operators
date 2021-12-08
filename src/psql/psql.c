#include "psql.h"

#include <string.h>
#include <stdlib.h>

#include "../debug.h"

#define MAX_QUERY_SIZE 256

PGconn *connect_to_database(const char *connection_info) {
    PGconn *connection = PQconnectdb(connection_info);

    if (PQstatus(connection) != CONNECTION_OK) {
        UFO_REPORT("Connection to database failed: %s", PQerrorMessage(connection));
        PQfinish(connection);
        return NULL;
    }

    return connection;
}

void disconnect_from_database(PGconn *connection) {
    PQfinish(connection);
}

int start_transaction(PGconn *connection) {
    PGresult *result = PQexec(connection, "BEGIN");

    if (PQresultStatus(result) != PGRES_COMMAND_OK) {
        UFO_REPORT("BEGIN command failed: %s", PQerrorMessage(connection));
        PQclear(result);
        PQfinish(connection);
        return 1;     
    }

    PQclear(result);
    return 0;
}

int end_transaction(PGconn *connection) {
    PGresult *result = PQexec(connection, "END");

    if (PQresultStatus(result) != PGRES_COMMAND_OK) {
        UFO_REPORT("END command failed: %s", PQerrorMessage(connection));
        PQclear(result);
        PQfinish(connection);
        return 1;
    }

    PQclear(result);
    return 0;
}

int retrieve_size_of_table(PGconn *connection, const char* table, size_t* count_return) {

    char query[MAX_QUERY_SIZE];
    sprintf(query, "SELECT count(*) FROM '%s'", table);
    UFO_LOG("Executing %s\n", query);
    PGresult *result = PQexec(connection, query);

    if (PQresultStatus(result) != PGRES_TUPLES_OK) {
        UFO_REPORT("Query failed (%s): %s\n", PQerrorMessage(connection), query);
        PQclear(result);
        PQfinish(connection);
        return 1;
    }

    char *count_string = PQgetvalue(result, 0, 0);
    (*count_return) = atol(count_string);

    PQclear(result);
    return 0;
}

psql_column_type_t psql_column_type_from(char* type) {

    if (strcmp(type, "integer"))           return PSQL_COL_INT;
    if (strcmp(type, "smallint"))          return PSQL_COL_INT;
    if (strcmp(type, "smallint"))          return PSQL_COL_INT;

    if (strcmp(type, "bool"))              return PSQL_COL_BOOL;

    if (strcmp(type, "character"))         return PSQL_COL_CHAR;
    if (strcmp(type, "character varying")) return PSQL_COL_CHAR;    
    if (strcmp(type, "text"))              return PSQL_COL_CHAR;

    if (strcmp(type, "real"))              return PSQL_COL_REAL;

    if (strcmp(type, "bytea"))             return PSQL_COL_BYTE;

    UFO_REPORT("Unsupported column type: %s", type);
    return PSQL_COL_UNSUPPORTED;
}

int retrieve_type_of_column(PGconn *connection, const char* table, const char* column, psql_column_type_t *type) {

    char query[MAX_QUERY_SIZE];
    sprintf(query, "SELECT data_type FROM information_schema.columns " 
                   "WHERE column_name = '%s' AND table_name = '%s'", column, table);

    UFO_LOG("Executing %s\n", query);
    PGresult *result = PQexec(connection, query);

    if (PQresultStatus(result) != PGRES_TUPLES_OK) {
        UFO_REPORT("Query failed (%s): %s\n", PQerrorMessage(connection), query);
        PQclear(result);
        PQfinish(connection);
        return 1;
    }

    char *type_string = PQgetvalue(result, 0, 0);
    (*type) = psql_column_type_from(type_string);

    PQclear(result);
    return 0;
}

int retrieve_from_table(PGconn *connection, const char* table, const char* column, uintptr_t start, uintptr_t end, psql_action action, unsigned char *target) {
    char query[256];
    sprintf(query, "SELECT %s FROM (SELECT row_number() OVER () nth, %s FROM %s) AS numbered "
                   "WHERE numbered.nth >= %li AND numbered.nth < %li", 
                   column, column, table, start + 1, end + 1); // 1-indexed
    
    UFO_LOG("Executing %s\n", query);
    PGresult *result = PQexec(connection, query);

    if (PQresultStatus(result) != PGRES_TUPLES_OK) {
        UFO_REPORT("SELECT command failed: %s\n", PQerrorMessage(connection));
        PQclear(result);
        PQfinish(connection);
        return 1;
    }

    int retrieved_rows = PQntuples(result);
    for (int i = 0; i < retrieved_rows; i++) {          
        char *element = PQgetvalue(result, i, 0);
        UFO_LOG("Loading object %i: %s\n", i, element);
        bool missing = PQgetisnull(result, i,0) == 1;
        int result = action(start + i, i, target, element, missing);
        if (result != 0) {
            return result;
        }
    }
    return 0;
}

