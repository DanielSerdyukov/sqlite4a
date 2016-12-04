# SQLite4a [![Apache License](https://img.shields.io/badge/license-Apache%20v2-blue.svg)](https://github.com/DanielSerdyukov/sqlite4a/blob/master/LICENSE) [![Build Status](https://gitlab.exzogeni.com/android/sqlite4a/badges/master/build.svg)](https://github.com/DanielSerdyukov/sqlite4a)

Simple jni wrapper for SQLite.

----

### Gradle
```groovy
compile 'sqlite4a:library:3.15.2-r3'
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
db.exec("PRAGMA case_sensitive_like = true;", null);

db.exec("SELECT sqlite_version() AS version;", (columns, values) -> {
    Log.i("SQLITE", Arrays.toString(columns));  // ["version"]
    Log.i("SQLITE", Arrays.toString(values));   // ["3.15.2"]
});
```

#### Prepared query
```java
SQLiteStmt stmt = db.prepare("INSERT INTO users(name, age) VALUES(?, ?);");
stmt.bindString(1, "John");
stmt.bindLong(2, 25);
long johnId = stmt.executeInsert();

stmt.clearBindings();

stmt.bindString(1, "Jane");
stmt.bindLong(2, 20);
long janeId = stmt.executeInsert();

stmt.close();
```

#### Query tracing
```java
db.trace(sql -> { Log.i("SQLITE", sql); });
db.exec("CREATE TABLE IF NOT EXISTS foo(a INTEGER PRIMARY KEY, b REAL, c TEXT, d BLOB, e TEXT);");
SQLite stmt = db.prepare("INSERT INTO foo(a, b, c, d, e) VALUES(?, ?, ?, ?, ?);");
// ... bind args
stmt.executeInsert();
stmt.close();
```
LogCat:
```
D/SQLITE: CREATE TABLE IF NOT EXISTS foo(a INTEGER PRIMARY KEY, b REAL, c TEXT, d BLOB, e TEXT);
D/SQLITE: INSERT INTO foo(a, b, c, d, e) VALUES(1000, 1.23, 'test1', x'010203', NULL);
```

#### Custom collation
```java
mDb.exec("CREATE TABLE test(name TEXT);", null);
mDb.exec("INSERT INTO test VALUES('John Doe');", null);
mDb.createCollation("NOCASE", new Comparator<String>() {
    @Override
    public int compare(String lhs, String rhs) {
        return lhs.compareToIgnoreCase(rhs);
    }
});
final SQLiteStmt stmt = mDb.prepare("SELECT * FROM test WHERE name = ? COLLATE NOCASE;");
stmt.bindString(1, "john doe");
final SQLiteCursor cursor = stmt.executeQuery();
Assert.assertThat(cursor.step(), Is.is(true));
Assert.assertThat(cursor.getColumnString(0), Is.is("John Doe"));
stmt.close();
```

#### Custom functions
```java
db.exec("CREATE TABLE test(name TEXT, age INTEGER, lat REAL, lng REAL, bytes BLOB);", null);
db.exec("INSERT INTO test VALUES('John', 25, 59.9388333, 30.315866, x'0103020405');", null);
db.createFunction("distanceBetween", 4, new SQLiteFunc() { // name, numArgs, func
   @Override
   public void call(@NonNull SQLiteContext context, @NonNull SQLiteValue[] values) {
       final float[] results = new float[1];
       Location.distanceBetween(values[0].doubleValue(), values[1].doubleValue(),
               values[2].doubleValue(), values[3].doubleValue(), results);
       context.resultDouble(results[0]);
   }
});
final SQLiteStmt stmt = mDb.prepare("SELECT distanceBetween(lat, lng, ?, ?) FROM test;");
stmt.bindDouble(1, lat);
stmt.bindDouble(2, lng);
final SQLiteCursor cursor = stmt.executeSelect();
while(cursor.step()) {
    Log.d(TAG, "distance = " + cursor.getColumnDouble(0);)
}
```

License
-------

    Copyright 2015-2016 Daniel Serdyukov

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
