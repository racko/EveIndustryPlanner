create table history (
  header integer primary_key,
  date integer not null,
  orders integer not null check(orders >= 0),
  low float not null check(low >= 0),
  high float not null check(high >= 0),
  average float not null check(average >= 0),
  quantity integer not null check(quantity >= 0),
  constraint header_foreign_key foreign key (header) references rowset_headers(id)
);
