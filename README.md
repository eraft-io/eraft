[![License](https://img.shields.io/badge/license-MIT-green)](https://opensource.org/licenses/MIT)

中文 | [English](README_en.md)

## 运行 raft kv 服务复制组

```
./target/debug/eraftkv-ctl 0 '[::1]:8088'
./target/debug/eraftkv-ctl 1 '[::1]:8089'
./target/debug/eraftkv-ctl 2 '[::1]:8090'
```

## 运行 proxy 服务
```
./target/debug/eraftproxy-ctl '127.0.0.1:8080' 'http://[::1]:8088,http://[::1]:8089,http://[::1]:8090'
```

## 连接 proxy 执行 SQL

telnet [proxy 地址]

操作示例

```
colin@colindeMacBook-Pro eraft % telnet 127.0.0.1 8080
Trying 127.0.0.1...
Connected to localhost.
Escape character is '^]'.
SHOW DATABASES;
ok
SELECT * FROM classtab WHERE Name = 'Tom';
|Name|Class|Score|
|Tom|B|88|
```

## 支持的 SQL 

### CREATE DATABASE

```
CREATE DATABASE testdb_1;
```

### SHOW DATABASES
```
SHOW DATABASES;
```

### CREATE TABLE

```
CREATE TABLE classtab ( Name VARCHAR(100), Class VARCHAR(100), Score INT, PRIMARY KEY(Name));
```

### SHOW CREATE TABLE

```
SHOW CREATE TABLE classtab;
```

### INSERT INTO

```
INSERT INTO classtab (Name,Class,Score) VALUES ('Tom', 'B', '88');
```

### SELECT BY PRIMARY KEY

```
SELECT * FROM classtab WHERE Name = 'Tom';
```
