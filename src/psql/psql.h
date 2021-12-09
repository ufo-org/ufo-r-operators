#pragma once

#include <libpq-fe.h> // For PSQL
#include <stdint.h>
#include <stdbool.h>

typedef enum {
    PSQL_COL_UNSUPPORTED,
    PSQL_COL_INT,
    PSQL_COL_BOOL,
    PSQL_COL_CHAR,
    PSQL_COL_REAL,
    PSQL_COL_BYTE,
} psql_column_type_t;

typedef int (*psql_read)(uintptr_t index_in_vector, int index_in_target, unsigned char *target, char *element, bool missing);
typedef int (*psql_write)(uintptr_t index_in_vector, int index_in_target, const unsigned char *contents, char *buffer, bool *missing);

PGconn *connect_to_database(const char *connection_info);
void disconnect_from_database(PGconn *connection);
int start_transaction(PGconn *connection);
int end_transaction(PGconn *connection);

int create_table_column_subscript(PGconn *connection, const char* table, const char *pk, const char* column);
int retrieve_from_table(PGconn *connection, const char* table, const char* column, uintptr_t start, uintptr_t end, psql_read action, unsigned char *target);
int update_table(PGconn *connection, const char* table, const char* column, const char* pk, uintptr_t start, uintptr_t end, psql_write write_action, const unsigned char *contents);
int retrieve_type_of_column(PGconn *connection, const char *table, const char *column, psql_column_type_t *type);
int retrieve_size_of_table(PGconn *connection, const char *table, size_t *count_return);
char *retrieve_table_pk(PGconn *connection, const char* table, const char* column);
psql_column_type_t psql_column_type_from(char *type);

