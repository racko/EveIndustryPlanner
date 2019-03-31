create table message_headers (
  id integer primary key,
  currentTime integer not null,
  version text not null,
  generator integer,
  constraint generator_foreign_key foreign key (generator) references generators (id)
);
