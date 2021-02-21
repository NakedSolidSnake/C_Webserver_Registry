FROM debian
RUN apt-get update -y
RUN apt-get install build-essential -y
RUN apt-get install git -y
RUN apt-get install systemd -y
RUN apt-get install libsystemd-dev -y
RUN apt-get install wget -y
RUN apt-get install mariadb-server -y
RUN apt-get install cmake -y
RUN apt-get install libmicrohttpd-dev -y
RUN apt-get install curl -y
RUN apt-get install libcurl4-gnutls-dev -y
RUN apt-get install libjansson-dev -y
RUN apt-get install libjson-c-dev -y
RUN apt-get install vim -y
RUN apt-get install zlib1g-dev -y
RUN apt-get install default-libmysqlclient-dev -y

# install ulfius
RUN git clone https://github.com/babelouest/ulfius.git
RUN cd ulfius && mkdir build && cd build && cmake .. && make && make install && ldconfig

# install middleware
RUN git clone https://github.com/NakedSolidSnake/middleware.git
RUN cd middleware && mkdir build && cd build && cmake .. && make && make install && ldconfig

# install webregistry
COPY . /webserver 
RUN cd webserver && mkdir build && cd build && cmake .. && make 

# run script to create database

# RUN service mysql start
# RUN service mysql status

# RUN mysql -u root -p < /webserver/scripts_sql/create_database.sql

EXPOSE 8095

# WORKDIR /webserver/script

# CMD /webserver/scripts/run.sh