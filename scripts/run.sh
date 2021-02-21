#!/bin/bash

service mysql start
mysql -u root -p Registry < /webserver/scripts_sql/create_database.sql
cd /webserver/build/bin/
./webserver