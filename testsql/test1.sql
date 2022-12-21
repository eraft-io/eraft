CREATE DATABASE db;

USE db;

CREATE TABLE Persons (
   PersonID int PRIMARY KEY, 
   LastName varchar(20), 
   FirstName varchar(20), 
   Address varchar(20), 
   City varchar(10)
);

CREATE INDEX Persons(PersonID);
CREATE INDEX Persons(FirstName);
INSERT INTO Persons VALUES 
(23, 'Yi', '测试', 'Tsinghua Univ.', 'Beijing'), 
(-238, 'Zhong', 'Lei', 'Beijing Univ.', 'Neijing'),
(1+999, 'Wasserstein', 'Zhang', 'Hunan Univ.', 'Hunan');

INSERT INTO Persons VALUES (1, 'Yi', '测试', 'Tsinghua Univ.', 'Beijing'), 
(3, 'Zhong', 'Lei', 'Beijing Univ.', 'Neijing'),
(4, 'Wasserstein', 'Zhang', 'Hunan Univ.', 'Hunan');

SELECT PersonID, LastName, FirstName, Address, City FROM Persons;

SELECT PersonID, LastName, FirstName FROM Persons WHERE FirstName = '测试'; 

UPDATE Persons SET FirstName = 'CxSpace' WHERE PersonID = 3;

SELECT * FROM Persons;

INSERT INTO Persons VALUES 
(100001, 'Zarisk', 'C', 'Unknown', 'US'), 
(100002, 'Wasserstein', 'D', 'Unknwon.', 'EU');

DELETE FROM Persons WHERE PersonID < 0;
SELECT * FROM Persons;
INSERT INTO Persons 
(LastName, PersonID) 
VALUES ('Zarisk', 10), 
       ('1999-10-10', 30), 
       ('Wasserstein', 20);
       
SELECT * FROM Persons;
SELECT COUNT(*) FROM Persons;
SELECT SUM(PersonID) FROM Persons;
SELECT AVG(PersonID) FROM Persons;
SELECT PersonID, LastName FROM Persons;
