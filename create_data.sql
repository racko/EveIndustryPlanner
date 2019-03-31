drop table accounts;
drop table transactions;
drop table flows;

create table accounts (
    id integer primary key,
    type integer not null references types,
    description text not null unique
);

create table transactions (
    id integer primary key,
    date integer not null,
    description text not null unique
);

create table flows (
    id integer primary key,
    transaction_id integer not null references transactions,
    source_account integer not null references accounts,
    target_account integer not null references accounts,
    source_volume real not null,
    target_volume real not null,

    check (source_account != target_account)
    --constraint source_foreign_key foreign key (source_account) references accounts(id),
    --constraint target_foreign_key foreign key (target_account) references accounts(id)
);
