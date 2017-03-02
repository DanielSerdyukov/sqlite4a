# SQLite4a [![Apache License](https://img.shields.io/badge/license-Apache%20v2-blue.svg)](https://github.com/DanielSerdyukov/sqlite4a/blob/master/LICENSE) [![Build Status](https://gitlab.exzogeni.com/android/sqlite4a/badges/master/build.svg)](https://github.com/DanielSerdyukov/sqlite4a)

Simple jni wrapper for SQLite.

----

### Gradle
```groovy
compile 'sqlite4a:sqlite4a:3.17.0'
```

### Some examples

#### Open database
```java
SQLiteDb db = SQLite.open(":memory:", SQLite.OPEN_READWRITE | SQLite.OPEN_CREATE);
```
or
```java
SQLiteDb db = SQLite.open("/absolute/path", SQLite.OPEN_READWRITE | SQLite.OPEN_CREATE);
```

#### Simple query
```java
db.exec("PRAGMA case_sensitive_like = true;");
```

#### Prepared query
```java
SQLiteStmt stmt = db.prepare("INSERT INTO users(name, age) VALUES(?, ?);");
stmt.bindString(1, "John");
stmt.bindLong(2, 25);
long johnId = stmt.insert();

stmt.clearBindings();

stmt.bindString(1, "Jane");
stmt.bindLong(2, 20);
long janeId = stmt.insert();

stmt.close();
```

#### Query tracing
```java
db.trace(sql -> { Log.i("SQLITE", sql); });
db.exec("CREATE TABLE IF NOT EXISTS foo(a INTEGER PRIMARY KEY, b REAL, c TEXT, d BLOB, e TEXT);");
SQLite stmt = db.prepare("INSERT INTO foo(a, b, c, d, e) VALUES(?, ?, ?, ?, ?);");
// ... bind args
stmt.insert();
stmt.close();
```
LogCat:
```
D/SQLITE: CREATE TABLE IF NOT EXISTS foo(a INTEGER PRIMARY KEY, b REAL, c TEXT, d BLOB, e TEXT);
D/SQLITE: INSERT INTO foo(a, b, c, d, e) VALUES(1000, 1.23, 'test1', x'010203', NULL);
```

#### Custom collation
```java
mDb.exec("CREATE TABLE test(name TEXT);");
mDb.exec("INSERT INTO test VALUES('lorem ipsum');");
mDb.createCollation("JAVA_NOCASE", new Comparator<String>() {
    @Override
    public int compare(String lhs, String rhs) {
        return lhs.compareToIgnoreCase(rhs);
    }
});
final SQLiteStmt stmt = mDb.prepare("SELECT * FROM test WHERE name = ? COLLATE JAVA_NOCASE;");
stmt.bindString(1, "LOREM ipsum");
try (final SQLiteIterator iterator = stmt.select()) {
    Assert.assertThat(iterator.hasNext(), Is.is(true));
    Assert.assertThat(iterator.next().getColumnString(0), Is.is("lorem ipsum"));
}
```

#### Custom functions
```java
db.exec("CREATE TABLE test(first_name TEXT, last_name INTEGER);");
db.exec("INSERT INTO test VALUES('John', 'Smith');");
db.createFunction("join_name", 2, (context, values) -> {
    context.resultString(values[0].stringValue() + " " + values[1].stringValue());
});
try (final SQLiteIterator iterator = mDb.prepare("SELECT join_name(first_name, last_name) FROM test;").select()) {
    Assert.assertThat(iterator.hasNext(), Is.is(true));
    Assert.assertThat(iterator.next().getColumnString(0), IsEqual.equalTo("John Smith"));
}
```

License
-------

    Copyright 2016-2017 exzogeni.com

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
