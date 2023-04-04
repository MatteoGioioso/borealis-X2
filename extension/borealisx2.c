#include "postgres.h"
#include "fmgr.h"
#include "utils/builtins.h"
#include "executor/execdesc.h"
#include "executor/executor.h"
#include "utils/elog.h"
#include "borealisx2.h"

PG_MODULE_MAGIC;

void _PG_init(void);
void _PG_fini(void);

PGDLLEXPORT Datum version(PG_FUNCTION_ARGS);

PG_FUNCTION_INFO_V1(version);

static ExecutorStart_hook_type PrevExecutorStart_hook = NULL;
static void bx2_executor_start(QueryDesc *queryDesc, int eflags);


Datum
version(PG_FUNCTION_ARGS)
{
    PG_RETURN_TEXT_P(cstring_to_text("0.0.1"));
}

/*
 * Entrypoint of this module - called at shared_preload_libraries time in the
 * context of the postmaster.
 */
void
_PG_init(void)
{
	elog(LOG, "Starting Borealisx2");

	// PrevExecutorStart_hook = ExecutorStart_hook;
	// ExecutorStart_hook = bx2_executor_start;
}

/*
 * The BDR ExecutorStart_hook that does DDL lock checks and forbids
 * writing into tables without replica identity index.
 *
 * Runs in all backends and workers.
 */
static void
bx2_executor_start(QueryDesc *queryDesc, int eflags)
{	
	elog(LOG, "hook called");
	standard_ExecutorStart(queryDesc, eflags);
}


void
_PG_fini(void)
{
	ExecutorStart_hook = PrevExecutorStart_hook;
}