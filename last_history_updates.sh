#! /bin/sh
sqlite3 market_history.db "select datetime(min(date), 'unixepoch', 'localtime') from last_history_update; select datetime(max(date), 'unixepoch', 'localtime') from last_history_update;"
