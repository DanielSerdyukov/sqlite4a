/*
 * Copyright 2016-2017 exzogeni.com
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package sqlite4a;

import android.support.test.InstrumentationRegistry;
import android.support.test.runner.AndroidJUnit4;

import com.getkeepsafe.relinker.ReLinker;

import org.hamcrest.collection.IsIterableContainingInOrder;
import org.hamcrest.core.Is;
import org.junit.Assert;
import org.junit.BeforeClass;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.ArrayList;
import java.util.List;

@RunWith(AndroidJUnit4.class)
public class SQLiteDbTest {

    @BeforeClass
    public static void loadLibrary() {
        ReLinker.loadLibrary(InstrumentationRegistry.getContext(), SQLite.JNI_LIB);
    }

    @Test
    public void openReadOnly() throws Exception {
        final SQLiteDb db = SQLite.open(":memory:", SQLite.OPEN_READONLY);
        Assert.assertThat(db.isReadOnly(), Is.is(true));
        db.close();
    }

    @Test
    public void openReadWrite() throws Exception {
        final SQLiteDb db = SQLite.open(":memory:", SQLite.OPEN_READWRITE);
        Assert.assertThat(db.isReadOnly(), Is.is(false));
        db.close();
    }

    @Test
    public void inTransaction_Begin_Commit() throws Exception {
        final SQLiteDb db = SQLite.open(":memory:", SQLite.OPEN_READWRITE);
        Assert.assertThat(db.inTransaction(), Is.is(false));
        db.exec("BEGIN;");
        Assert.assertThat(db.inTransaction(), Is.is(true));
        db.exec("COMMIT;");
        Assert.assertThat(db.inTransaction(), Is.is(false));
        db.close();
    }

    @Test
    public void inTransaction_Begin_Rollback() throws Exception {
        final SQLiteDb db = SQLite.open(":memory:", SQLite.OPEN_READWRITE);
        Assert.assertThat(db.inTransaction(), Is.is(false));
        db.exec("BEGIN;");
        Assert.assertThat(db.inTransaction(), Is.is(true));
        db.exec("ROLLBACK;");
        Assert.assertThat(db.inTransaction(), Is.is(false));
        db.close();
    }

    @Test
    public void trace() throws Exception {
        final SQLiteDb db = SQLite.open(":memory:", SQLite.OPEN_READWRITE | SQLite.OPEN_CREATE);
        final List<String> traceLog = new ArrayList<>();
        db.trace(new SQLiteDb.Trace() {
            @Override
            public void trace(String sql) {
                traceLog.add(sql);
            }
        });
        db.exec("BEGIN;");
        db.exec("CREATE TABLE IF NOT EXISTS foo(text TEXT PRIMARY KEY);");
        db.exec("CREATE TABLE IF NOT EXISTS bar(text TEXT REFERENCES foo(text) ON DELETE CASCADE);");
        db.exec("INSERT INTO foo VALUES('test');");
        db.exec("INSERT INTO bar VALUES('test');");
        db.exec("INSERT INTO bar VALUES('test');");
        db.exec("COMMIT;");
        db.exec("DELETE FROM foo;");
        Assert.assertThat(traceLog, IsIterableContainingInOrder.contains(
                "BEGIN;",
                "CREATE TABLE IF NOT EXISTS foo(text TEXT PRIMARY KEY);",
                "CREATE TABLE IF NOT EXISTS bar(text TEXT REFERENCES foo(text) ON DELETE CASCADE);",
                "INSERT INTO foo VALUES('test');",
                "INSERT INTO bar VALUES('test');",
                "INSERT INTO bar VALUES('test');",
                "COMMIT;",
                "DELETE FROM foo;",
                "-- TRIGGER "
        ));
        db.close();
    }

    @Test
    public void execForNumber() throws Exception {
        final SQLiteDb db = SQLite.open(":memory:", SQLite.OPEN_READWRITE | SQLite.OPEN_CREATE);
        db.exec("PRAGMA user_version = 123;");
        Assert.assertThat(db.execForNumber("PRAGMA user_version;").longValue(), Is.is(123L));
        db.close();
    }

}