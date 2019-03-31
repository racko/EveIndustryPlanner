CREATE TABLE systems (
    systemid integer primary key,
    constellationid integer not null references constellations,
    name text not null
);
