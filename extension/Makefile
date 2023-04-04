EXTENSION = borealisx2        # the extensions name
DATA = borealisx2--0.0.1.sql  # script files to install
MODULE_big = borealisx2
OBJS = borealisx2_node_init.o borealisx2_utils.o borealisx2.o
PG_CPPFLAGS = -I$(libpq_srcdir)
SHLIB_LINK = $(libpq)

# postgres build stuff
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)