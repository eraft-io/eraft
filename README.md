[![License](https://img.shields.io/badge/license-MIT-green)](https://opensource.org/licenses/MIT)

中文 | [English](README_en.md)

### run kv raft group server

```
./target/debug/eraftkv-ctl 0 '[::1]:8088'
./target/debug/eraftkv-ctl 1 '[::1]:8089'
./target/debug/eraftkv-ctl 2 '[::1]:8090'
```

### run insert sql

```
./target/debug/eraftproxy-ctl 'http://[::1]:8088' "INSERT INTO classtab (
                                                    Name,
                                                    Class,
                                                    Age
                                                    )
                                                    VALUES
                                                    ('Tom',
                                                    'BXCCC',
                                                    '18');"

```

### run select sql
```
./target/debug/eraftproxy-ctl 'http://[::1]:8088' "SELECT * FROM classtab WHERE Name = 'Tom';"

```

