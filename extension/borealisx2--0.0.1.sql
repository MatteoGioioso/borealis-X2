-- complain if script is sourced in psql, rather than via CREATE EXTENSION
--\echo Use "CREATE EXTENSION borealisx2" to load this file. \quit

--CREATE ROLE bdr NOLOGIN SUPERUSER;
--SET ROLE bdr;

CREATE SCHEMA borealisx2;
GRANT USAGE ON SCHEMA borealisx2 TO public;

SET LOCAL search_path = borealisx2;

CREATE OR REPLACE FUNCTION borealisx2.version()
    RETURNS TEXT
    LANGUAGE C
AS 'MODULE_PATHNAME';