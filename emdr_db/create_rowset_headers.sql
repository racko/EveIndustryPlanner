create table rowset_headers (
  id integer primary key,
  message integer,
  generatedAt integer not null,
  regionID integer,
  typeID integer not null,
  constraint message_foreign_key foreign key (message) references message_headers(id)
);
