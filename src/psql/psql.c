#include "psql.h"

#include <string.h>
#include <stdlib.h>

#include "../debug.h"

#define MAX_VALUE_SIZE 50
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
    sprintf(query, "SELECT count(*) FROM %s", table);
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

    // printf("type=%s\n",type);

    if (strcmp(type, "integer") == 0)           return PSQL_COL_INT;
    if (strcmp(type, "smallint") == 0)          return PSQL_COL_INT;
    if (strcmp(type, "smallint") == 0)          return PSQL_COL_INT;

    if (strcmp(type, "bool") == 0)              return PSQL_COL_BOOL;
    if (strcmp(type, "boolean") == 0)           return PSQL_COL_BOOL;

    if (strcmp(type, "character") == 0)         return PSQL_COL_CHAR;
    if (strcmp(type, "character varying") == 0) return PSQL_COL_CHAR;    
    if (strcmp(type, "text") == 0)              return PSQL_COL_CHAR;

    if (strcmp(type, "real") == 0)              return PSQL_COL_REAL;

    //since it's not a single byte, i can;t really parse it.
    //if (strcmp(type, "bytea") == 0)             return PSQL_COL_BYTE;

    UFO_REPORT("Unsupported column type: %s", type);
    return PSQL_COL_UNSUPPORTED;
}

// Assuming just one
char *retrieve_table_pk(PGconn *connection, const char* table, const char* column) {
    char query[MAX_QUERY_SIZE];
    sprintf(query, "SELECT a.attname, format_type(a.atttypid, a.atttypmod) AS data_type "
                   "FROM   pg_index i "
                   "JOIN   pg_attribute a ON a.attrelid = i.indrelid "
                   "                     AND a.attnum = ANY(i.indkey) "
                   "WHERE  i.indrelid = '%s'::regclass "
                   "AND    i.indisprimary;", table);

    UFO_LOG("Executing %s\n", query);
    PGresult *result = PQexec(connection, query);

    if (PQresultStatus(result) != PGRES_TUPLES_OK) {
        UFO_REPORT("Query failed (%s): %s\n", PQerrorMessage(connection), query);
        PQclear(result);
        PQfinish(connection);
        return NULL;
    }

    int retrieved_rows = PQntuples(result);
    if (retrieved_rows == 0) {
        UFO_REPORT("Table %s has no PKs. UFOs need exactly one PK column to work.", table);
        PQclear(result);
        PQfinish(connection);
        return NULL;
    }
    if (retrieved_rows > 1) {
        UFO_REPORT("Table %s has %i PKs. UFOs need exactly one PK column to work.", table, retrieved_rows);
        PQclear(result);
        PQfinish(connection);
        return NULL;
    }

    char *pk = strdup(PQgetvalue(result, 0, 0));
    
    PQclear(result);
    return pk;  
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

int create_table_column_subscript(PGconn *connection, const char* table, const char *pk, const char* column) {
    char query[MAX_QUERY_SIZE];
    sprintf(query, "CREATE OR REPLACE VIEW \"ufo_%s_%s_subscript\" AS "
                   "(SELECT row_number() OVER (ORDER BY %s) - 1 nth, %s, %s FROM %s)", 
                   table, column, pk, pk, column, table); 
    
    UFO_LOG("Executing %s\n", query);
    PGresult *result = PQexec(connection, query);

    if (PQresultStatus(result) != PGRES_COMMAND_OK) {
        UFO_REPORT("CREATE VIEW command failed: [%x] '%s' (%s)\n", PQresultStatus(result), PQresultErrorMessage(result), query);
        PQclear(result);
        PQfinish(connection);
        return 1;
    }

    PQclear(result);
    return 0;
}

int retrieve_from_table(PGconn *connection, const char* table, const char* column, uintptr_t start, uintptr_t end, psql_read action, unsigned char *target) {
    char query[MAX_QUERY_SIZE];
    sprintf(query, "SELECT %s FROM ufo_%s_%s_subscript WHERE nth >= %li AND nth < %li", 
                   column, table, column, start, end); // 0-indexed
    
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
        int action_result = action(start + i, i, target, element, missing);
        if (action_result != 0) {
            PQclear(result);
            PQfinish(connection);
            return action_result;
        }
    }

    PQclear(result);
    return 0;
}

int update_table(PGconn *connection, const char* table, const char* column, const char* pk, uintptr_t start, uintptr_t end, psql_write write_action, const unsigned char *contents) {
    // TODO This could definitely have been implemented to be faster :/
    int result = start_transaction(connection);
    if (result != 0) {
        return result;
    }

    for (uintptr_t i = 0; i < end - start; i++) {
        char new_value[MAX_VALUE_SIZE];
        bool missing;
        int action_result = write_action(start + i, i, contents, new_value, &missing);
        if (action_result != 0) {
            return action_result;
        }

        char query[MAX_QUERY_SIZE];
        if (missing) {
            sprintf(query, "WITH subscript AS (SELECT %s, %s FROM ufo_%s_%s_subscript WHERE nth = %li) "
                            "UPDATE %s SET %s = NULL FROM subscript WHERE %s.%s = subscript.%s",
                            pk, column, table, column, i, table, column, table, pk, pk);
        } else {
            sprintf(query, "WITH subscript AS (SELECT %s, %s FROM ufo_%s_%s_subscript WHERE nth = %li) "
                            "UPDATE %s SET %s = '%s' FROM subscript WHERE %s.%s = subscript.%s",
                            pk, column, table, column, i, table, column, new_value, table, pk, pk);
        }

        UFO_LOG("Executing %s\n", query);
        PGresult *result = PQexec(connection, query);

        if (PQresultStatus(result) != PGRES_COMMAND_OK) {
            UFO_REPORT("UPDATE command failed: %s\n", PQerrorMessage(connection));
            PQclear(result);
            PQfinish(connection);
            return i + 1;
        }

        PQclear(result);
    }

    result = end_transaction(connection);
    if (result != 0) {
        return result;
    }

    return 0;
}
