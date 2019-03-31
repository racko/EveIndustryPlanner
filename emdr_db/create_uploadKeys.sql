create table upload_keys (
  id integer primary key,
  name text,
  key text,
  constraint unique_keys unique (name, key)
);
