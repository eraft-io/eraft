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

数据行 <8, 'B', 82.3> 可以映射为下面的 kv

| ------------------ | -------- |
|  /classtab/8/Class |    B     |
|  /classtab/8/Point |    82.3  |
| ------------------ | -------- |

