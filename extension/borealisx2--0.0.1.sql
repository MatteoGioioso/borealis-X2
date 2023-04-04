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
AS
'MODULE_PATHNAME';

CREATE OR REPLACE FUNCTION borealisx2.borealisx2_node_init(clone_from_conn_dsn text)
    RETURNS VOID
    LANGUAGE C
AS
'MODULE_PATHNAME';

CREATE TABLE borealisx2_nodes
(
    cluster_name         text not null,
    node_id              text not null,
    node_remote_conn_dsn text not null,
    node_status          text not null,

    PRIMARY KEY (node_id)
);

-- https://www.postgresql.org/docs/current/logical-replication-security.html
-- If we want to join the node to a cluster we need to specify clone_conn_dsn, which is the dsn of the node in which we want to start
-- the replication
CREATE FUNCTION borealisx2.add_node(
    cluster_name text,
    node_remote_conn_dsn text,
    clone_from_conn_dsn text default null
)
    RETURNS void
    LANGUAGE plpgsql
    VOLATILE
    SET search_path = borealisx2
AS
$body$
DECLARE
    localinfo RECORD;
BEGIN
    IF cluster_name IS NULL THEN
        RAISE USING
            MESSAGE = 'you must specify a cluster name',
            ERRCODE = 'invalid_parameter_value';
    END IF;

    IF node_remote_conn_dsn IS NULL THEN
        RAISE USING
            MESSAGE = 'dsn may not be null',
            ERRCODE = 'invalid_parameter_value';
    END IF;

    SELECT * INTO localinfo FROM pg_control_system();

    -- Init node
    PERFORM borealisx2.borealisx2_node_init(clone_from_conn_dsn);

    INSERT INTO borealisx2.borealisx2_nodes (cluster_name, node_id, node_remote_conn_dsn, node_status)
    VALUES (cluster_name, localinfo.system_identifier, node_remote_conn_dsn, 'ready');
END;
$body$;

-- If it is the first node we need to create a cluster first
CREATE FUNCTION borealisx2.create_cluster(
    cluster_name text,
    node_remote_conn_dsn text
)
    RETURNS void
    LANGUAGE plpgsql
    VOLATILE
    SET search_path = borealisx2
AS
$body$
BEGIN
    IF cluster_name IS NULL THEN
        RAISE USING
            MESSAGE = 'you must specify a cluster name',
            ERRCODE = 'invalid_parameter_value';
    END IF;

    IF node_remote_conn_dsn IS NULL THEN
        RAISE USING
            MESSAGE = 'dsn may not be null',
            ERRCODE = 'invalid_parameter_value';
    END IF;

    PERFORM borealisx2.add_node(cluster_name, node_remote_conn_dsn);
END;
$body$;