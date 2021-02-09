CREATE DATABASE Registry;
USE Registry;

CREATE TABLE Person(
    id int AUTO_INCREMENT PRIMARY KEY,
    address VARCHAR(121) NOT NULL,
    name VARCHAR(121) NOT NULL,
    age int NOT NULL
);