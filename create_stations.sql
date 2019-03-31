CREATE TABLE stations (
    stationid integer primary key,
    name text not null,
    systemid integer not null references systems
);
