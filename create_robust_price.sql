create table robust_price (
    typeid integer primary key references types(typeid),
    buy float check(buy >= 0),
    sell float check(sell >= 0),
    date integer not null
);
