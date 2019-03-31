create table orders2 (
  orderID integer primary key,
  typeID integer not null,
  bid integer not null,
  price float not null check(price >= 0),
  volRemaining integer not null check(volRemaining >= 0),
  stationID integer not null,
  availableUntil integer not null check(availableUntil >= 0)
);

CREATE TABLE last_update(
  regionID INT not null,
  typeID INT not null,
  lastUpdate INT not null check(lastUpdate >= 0),
  primary key (regionID, typeID)
);

attach "eve.sqlite" as eve;
insert into last_update select distinct regionID, typeID, max(lastUpdate) from orders join staStations on staStations.stationID = orders.stationID group by regionID, typeID order by regionID, typeID;

insert into orders2 select orderID, typeID, bid, price, volRemaining, stationID, availableUntil from orders;

drop table orders;
alter table orders2 rename to orders;

create index stationAndType on orders (stationID, typeID);
