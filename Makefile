# general config
MODULES = postxml
EXTENSION = postxml
DATA = postxml--1.0.sql

# postgres build config
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)

LDFLAGS = -lxml2 -lxslt -I/usr/include/libxml2