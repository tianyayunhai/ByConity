SHOW CREATE DATABASE INFORMATION_SCHEMA;
SHOW CREATE INFORMATION_SCHEMA.COLUMNS;
SELECT create_table_query FROM system.tables WHERE database ILIKE 'INFORMATION_SCHEMA' AND name ILIKE 'TABLES'; -- suppress style check: database = currentDatabase(0)
