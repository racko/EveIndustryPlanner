for file in $(ls *.sql) ; do
  sqlite3 emdr.db < $file
done
