create table orders (
  orderID integer,
  header integer,
  price float not null check(price >= 0),
  volRemaining integer not null check(volRemaining >= 0),
  range integer not null,
  volEntered integer not null check(volEntered >= 0),
  minVolume integer not null check(minVolume >= 1),
  bid integer not null,
  issueDate integer not null,
  duration integer not null check(duration >= 0),
  stationID integer not null,
  solarSystemID integer not null,
  constraint primary_key primary key (orderID, header),
  constraint header_foreign_key foreign key (header) references rowset_headers (id)
);
