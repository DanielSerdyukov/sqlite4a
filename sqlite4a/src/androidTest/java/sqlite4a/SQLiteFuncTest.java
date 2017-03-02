/*
 * Copyright 2017 exzogeni.com
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

import android.support.annotation.NonNull;
import android.support.test.InstrumentationRegistry;
import android.support.test.runner.AndroidJUnit4;
import android.text.TextUtils;

import com.getkeepsafe.relinker.ReLinker;

import org.hamcrest.core.Is;
import org.hamcrest.core.IsEqual;
import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class SQLiteFuncTest {

    private SQLiteDb mDb;

    @BeforeClass
    public static void loadLibrary() {
        ReLinker.loadLibrary(InstrumentationRegistry.getContext(), SQLite.JNI_LIB);
    }

    @Before
    public void setUp() throws Exception {
        mDb = SQLite.open(":memory:", SQLite.OPEN_READWRITE | SQLite.OPEN_CREATE);
        mDb.exec("CREATE TABLE test(foo TEXT, bar TEXT, baz TEXT);");
        mDb.exec("INSERT INTO test VALUES('тест', 'capitalize', NULL);");
    }

    @Test
    public void override_upper() throws Exception {
        mDb.createFunction("upper", 1, new SQLiteDb.Func() {
            @Override
            public void call(@NonNull SQLiteContext context, @NonNull SQLiteValue[] values) {
                final String value = values[0].stringValue();
                if (value != null) {
                    context.resultString(value.toUpperCase());
                }
            }
        });
        final SQLiteIterator iterator = mDb.prepare("SELECT upper(foo) FROM test;").select();
        Assert.assertThat(iterator.hasNext(), Is.is(true));
        Assert.assertThat(iterator.next().getColumnString(0), IsEqual.equalTo("ТЕСТ"));
        iterator.close();
    }

    @Test
    public void custom_join() throws Exception {
        mDb.createFunction("custom_join", 2, new SQLiteDb.Func() {
            @Override
            public void call(@NonNull SQLiteContext context, @NonNull SQLiteValue[] values) {
                context.resultString(TextUtils.join(" ", new String[]{
                        values[0].stringValue(),
                        values[1].stringValue(),
                }));
            }
        });
        final SQLiteStmt stmt = mDb.prepare("UPDATE test SET baz = custom_join(?, ?);");
        stmt.bindString(1, "join");
        stmt.bindString(2, "us");
        stmt.execute();
        stmt.close();
        final SQLiteIterator iterator = mDb.prepare("SELECT baz FROM test;").select();
        Assert.assertThat(iterator.hasNext(), Is.is(true));
        Assert.assertThat(iterator.next().getColumnString(0), IsEqual.equalTo("join us"));
        iterator.close();
    }

    @After
    public void tearDown() throws Exception {
        mDb.close();
    }

}
