#include "postgres.h"
#include "executor/spi.h"
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>


int
borealisx2_utils_spi_getattno(const char *colname)
{
	int attno;

	attno = SPI_fnumber(SPI_tuptable->tupdesc, colname);
	if (attno == SPI_ERROR_NOATTRIBUTE)
		elog(ERROR, "SPI error while reading %s", colname);

	return attno;
}


void borealisx2_utils_sysid_get(char *sys_id)
{
    snprintf(sys_id, sizeof(sys_id), UINT64_FORMAT,
			 GetSystemIdentifier());
             
	sys_id[sizeof(sys_id)-1] = '\0';
}