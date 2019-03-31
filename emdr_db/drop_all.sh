for table in $(sqlite3 emdr.db <<< .tables) ; do
  cmd="drop table $table;"
  echo $cmd
  sqlite3 emdr.db <<< $cmd
done
