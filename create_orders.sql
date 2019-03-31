create table orders (
    orderid integer primary key,
    typeid integer not null references types,
    station integer not null references stations,
    volume_entered integer not null,
    duration integer not null,
    min_volume integer not null,
    active integer not null
);
CREATE INDEX orders_typeid on orders(typeid);
--CREATE INDEX orders_id_station on orders(orderid, station);
--CREATE INDEX orders_id_station on orders(station);

create table order_views (
    viewtime integer,
    orderid integer references orders,
    price float not null,
    volume integer not null check(volume >= 0),
    start_date integer not null, -- here because updating an order will keep the orderid but change the start date

    constraint order_views_primary_key primary key (viewtime, orderid)
);
CREATE INDEX order_views_id_time on order_views(orderid, viewtime);

create table buy_orders (
    orderid integer primary key references orders,
    range string not null
);

create table sell_orders (
    orderid integer primary key references orders
);

create table stations (
    stationid integer primary key,
    name text not null,
    regionid integer not null references regions
);
--CREATE INDEX stations_regionid on stations(regionid);

create table regions (
    regionid integer primary key,
    name text not null
);
