CREATE FUNCTION ailike RETURNS INTEGER SONAME 'ailike_udf.so';
INSTALL PLUGIN ailike_rewrite SONAME 'ailike_rewrite.so';
