CREATE TABLE constellations (
    constellationid integer primary key,
    regionid integer not null references regions,
    name text not null
);
