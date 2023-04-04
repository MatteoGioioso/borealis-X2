#include "postgres.h"
#include "borealisx2.h"
#include <inttypes.h>
#include <stdio.h>
#include "fmgr.h"
#include "utils/elog.h"
#include "postgresql/libpq-fe.h"
#include "utils/builtins.h"
#include "utils/memutils.h"
#include "executor/spi.h"
#include "lib/stringinfo.h"

PGDLLEXPORT Datum borealisx2_node_init(PG_FUNCTION_ARGS);

PG_FUNCTION_INFO_V1(borealisx2_node_init);

static bool xacthook_registered = false;

typedef struct {
	char *host;
	char *username;
	char *password;
	char *dbname;

} PostgresConnectionOptions;

static void borealisx2_xact_callback(XactEvent event, void *arg);
PostgresConnectionOptions connstr_get(char *connstr);
static Borealisx2Node* nodes_get();
static void node_publication_create(char *publication_name);
static void node_subscription_create(char *subscription_name, char *publication_name, char *clone_from_conn_dsn);
static Borealisx2Node node_remoteinfo_get(char *clone_from_conn_dsn);

Datum
borealisx2_node_init(PG_FUNCTION_ARGS)
{
    // If clone_from_conn_dsn is null means it is the first remote_node to join
    bool is_first_node = PG_ARGISNULL(0);
    char *clone_from_conn_dsn;
    static bool registered;
    PostgresConnectionOptions conn_options;
    char sys_id[33];
    Borealisx2Node remote_node;
    Borealisx2Node *nodes;
    int nodes_count;

    snprintf(sys_id, sizeof(sys_id), UINT64_FORMAT, GetSystemIdentifier());
	sys_id[sizeof(sys_id)-1] = '\0';

    elog(LOG, "init node %s", sys_id);

    if (is_first_node)
    {
        node_publication_create(sys_id);
        PG_RETURN_VOID();
    } else {
        clone_from_conn_dsn = text_to_cstring(PG_GETARG_TEXT_P(0));
        remote_node = node_remoteinfo_get(clone_from_conn_dsn);
        node_subscription_create(remote_node.node_id, remote_node.node_id, clone_from_conn_dsn);

        // Wait for nodes table to sync

        // Get all nodes and create subscription

        // nodes = nodes_get();
        // nodes_count = sizeof(nodes) / sizeof(nodes[0]);
        // for (size_t i = 0; i < nodes_count; i++)
        // {
        //     elog(LOG, "remote_node %s", nodes[i].node_id);
        //     node_subscription_create(nodes[i].node_id, nodes[i].node_id, nodes[i].node_remote_conn_dsn);
        // }
        // free(nodes);
        
        // Wait for ready

        // Wait for unknoledgement
        PG_RETURN_VOID();
    }
}


PostgresConnectionOptions
connstr_get(char *connstr)
{
	
	char	   *err_msg = NULL;
	PostgresConnectionOptions pg_connection_options;
	PQconninfoOption *conn_opts = NULL;
	PQconninfoOption *conn_opt;

	conn_opts = PQconninfoParse(connstr, &err_msg);
    if (err_msg != NULL)
    {
        elog(ERROR, "error parsing connection info %s", err_msg);
    }

	for (conn_opt = conn_opts; conn_opt->keyword != NULL; conn_opt++)
	{
        if (strcmp(conn_opt->keyword, "dbname") == 0)
        {
			pg_connection_options.dbname = conn_opt->val;
			continue;
        }
		if (strcmp(conn_opt->keyword, "host") == 0)
        {
			pg_connection_options.host = conn_opt->val;
			continue;
        }
		if (strcmp(conn_opt->keyword, "user") == 0)
        {
			pg_connection_options.username = conn_opt->val;
			continue;
        }
		if (strcmp(conn_opt->keyword, "password") == 0)
        {
            pg_connection_options.password = conn_opt->val;
            continue;
        }
	}

	return pg_connection_options;
}

static Borealisx2Node*
nodes_get()
{
	int	i;
    uint32 nprocessed;
	SPITupleTable *tuptable;
    const char *query;
    HeapTuple tuple;
    int	spi_ret;
	Oid	argtypes[] = { TEXTOID };
	Datum values[1];
	Oid	schema_oid;
    char sys_id_str[33];
    struct Borealisx2Node *nodes;

    elog(LOG, "getting nodes");

    snprintf(sys_id_str, sizeof(sys_id_str), UINT64_FORMAT, GetSystemIdentifier());
	sys_id_str[sizeof(sys_id_str)-1] = '\0';
    
    SPI_connect();

    values[0] = CStringGetTextDatum(&sys_id_str[0]);
    query = "SELECT node_id, node_remote_conn_dsn, node_status FROM borealisx2.borealisx2_nodes WHERE node_id != $1 AND node_status='ready'";
    spi_ret = SPI_execute_with_args(query, 1, argtypes, values, NULL, false, 0);
    if (spi_ret != SPI_OK_SELECT)
    {
        elog(ERROR, "SPI query failed: %d", spi_ret);
    }

	nprocessed = SPI_processed;
	tuptable = SPI_tuptable;

    nodes = malloc(nprocessed * sizeof(struct Borealisx2Node));

	for (i = 0; i < nprocessed; i++)
	{
        tuple = SPI_tuptable->vals[i];
		nodes[i].node_id = SPI_getvalue(tuple, SPI_tuptable->tupdesc, borealisx2_utils_spi_getattno("node_id"));
		nodes[i].node_remote_conn_dsn = SPI_getvalue(tuple, SPI_tuptable->tupdesc, borealisx2_utils_spi_getattno("node_remote_conn_dsn"));
		nodes[i].node_status = SPI_getvalue(tuple, SPI_tuptable->tupdesc, borealisx2_utils_spi_getattno("node_status"));
	}

    SPI_finish();

    return nodes;
}

static void
node_publication_create(char *publication_name)
{
    
    StringInfoData query;
    int	spi_ret;

    initStringInfo(&query);
    appendStringInfo(&query, "CREATE PUBLICATION %s%s", BOREALISX2_PUBLICATION_PREFIX, publication_name);
    appendStringInfo(&query, " FOR ALL TABLES");

    SPI_connect();

    spi_ret = SPI_exec(query.data, 0);

    if (spi_ret < 0)
    {
        elog(ERROR, "SPI query failed to create publication with code: %d", spi_ret);
    }

    SPI_finish();
}

static void
node_subscription_create(char *subscription_name, char *publication_name, char *clone_from_conn_dsn)
{
    StringInfoData query;
    int	spi_ret;

    initStringInfo(&query);
    appendStringInfo(&query, "CREATE SUBSCRIPTION %s%s", BOREALISX2_SUBSCRIPTION_PREFIX, subscription_name);
    appendStringInfo(&query, " CONNECTION '%s'", clone_from_conn_dsn);
    appendStringInfo(&query, " PUBLICATION '%s'", publication_name);

    SPI_connect();

    spi_ret = SPI_exec(query.data, 0);

    if (spi_ret < 0)
    {
        elog(ERROR, "SPI query failed to create subscription with code: %d", spi_ret);
    }

    SPI_finish();
}

static Borealisx2Node
node_remoteinfo_get(char *clone_from_conn_dsn)
{
    PGconn *remote_pg_conn;
    PGresult *res;
    char *remote_sysid;
    const char * values[1];
	Oid	types[1] = { TEXTOID };
    Borealisx2Node remote_node;
    
    remote_pg_conn = PQconnectdb(clone_from_conn_dsn);

    res = PQexec(remote_pg_conn, "SELECT system_identifier FROM pg_control_system()");
    if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
        PQclear(res);
        elog(ERROR, "Could get system identifier: %s", PQresultErrorMessage(res));
	}

    remote_sysid = PQgetvalue(res, 0, 0);
    PQclear(res);

    values[0] = remote_sysid;
    res = PQexecParams(remote_pg_conn, 
        "SELECT node_id, node_remote_conn_dsn, node_status FROM borealisx2.borealisx2_nodes WHERE node_id = $1",
        1, types, values, NULL, NULL, 0);

    if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
        PQclear(res);
        elog(ERROR, "Could get system identifier: %s", PQresultErrorMessage(res));
	}

    PQclear(res);
	PQfinish(remote_pg_conn);
	remote_pg_conn = NULL;

    remote_node.node_id = remote_sysid;
    remote_node.node_remote_conn_dsn = clone_from_conn_dsn;
    remote_node.node_status = PQgetvalue(res, 0, 2);

    return remote_node;
}