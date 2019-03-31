create table history (
    typeid integer not null,
    date integer not null check(date >= 0),
    region integer not null,
    orders integer not null check(orders >= 0),
    low float not null check(low >= 0),
    high float not null check(high >= 0),
    average float not null check(average >= 0),
    quantity integer not null check(quantity >= 0),

    constraint history_primary_key primary key (typeid, date, region),
    constraint typeid_foreign_key foreign key (typeid) references types(typeid),
    check (low <= avg),
    check (avg <= high),
    check (quantity >= orders)
);

create table last_history_update (
    typeid integer not null references types,
    region integer not null,
    date integer not null,

    constraint last_history_update_primary_key primary key (typeid, region)
);
