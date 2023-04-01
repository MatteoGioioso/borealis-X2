-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION borealisx2" to load this file. \quit
CREATE FUNCTION hello() RETURNS text
AS 'MODULE_PATHNAME'
    LANGUAGE C IMMUTABLE STRICT;