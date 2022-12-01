### 将 sql table 数据映射成 kv

```

CREATE TABLE classtab (
      ID       INT PRIMARY KEY,
      Class    STRING,
      Point    FLOAT,
)

INSERT INTO classtab (
                        ID,
                        Class,
                        Point
                    )
                        VALUES
                    (
                        8,
                        'B',
                        82.3
                    );
```

映射规格
```
/table_name[表名]/row_key[主键值]/feild_name[列名]  => feild_value [记录里面对应列的值]
```

数据行 <8, 'B', 82.3> 可以映射为下面的 kv

| ------------------ | -------- |
|  /classtab/8/Class |    B     |
|  /classtab/8/Point |    82.3  |
| ------------------ | -------- |

