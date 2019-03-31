create table orders (
  orderID integer primary key,
  typeID integer not null,
  bid integer not null,
  price float not null check(price >= 0),
  volRemaining integer not null check(volRemaining >= 0),
  stationID integer not null,
  availableUntil integer not null check(availableUntil >= 0)
  lastUpdate integer not null check(lastUpdate >= 0),
);

create index stationAndType on orders (stationID, typeID);
