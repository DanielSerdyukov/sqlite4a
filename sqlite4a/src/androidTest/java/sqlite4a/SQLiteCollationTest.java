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

import org.hamcrest.core.Is;
import org.hamcrest.core.IsEqual;
import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.Comparator;

@RunWith(AndroidJUnit4.class)
public class SQLiteCollationTest {

    private SQLiteDb mDb;

    @BeforeClass
    public static void loadLibrary() {
        ReLinker.loadLibrary(InstrumentationRegistry.getContext(), SQLite.JNI_LIB);
    }

    @Before
    public void setUp() throws Exception {
        mDb = SQLite.open(":memory:", SQLite.OPEN_READWRITE | SQLite.OPEN_CREATE);
        mDb.createCollation("JAVA_NOCASE", new Comparator<String>() {
            @Override
            public int compare(String lhs, String rhs) {
                return lhs.compareToIgnoreCase(rhs);
            }
        });
        mDb.exec("CREATE TABLE test(foo TEXT COLLATE JAVA_NOCASE, bar TEXT);");
        mDb.exec("INSERT INTO test VALUES('test', 'test');");
    }

    @Test
    public void tableCollation_Case_Insensitive_Column() throws Exception {
        final SQLiteIterator iterator = mDb.prepare("SELECT * FROM test WHERE foo = 'TEST';").select();
        Assert.assertThat(iterator.hasNext(), Is.is(true));
        Assert.assertThat(iterator.next().getColumnString(0), IsEqual.equalTo("test"));
        iterator.close();
    }

    @Test
    public void tableCollation_Case_Sensitive_Column() throws Exception {
        final SQLiteIterator iterator = mDb.prepare("SELECT * FROM test WHERE bar = 'TEST';").select();
        Assert.assertThat(iterator.hasNext(), Is.is(false));
        iterator.close();
    }

    @Test
    public void queryCollation() throws Exception {
        final SQLiteIterator iterator = mDb.prepare("SELECT * FROM test " +
                "WHERE bar = 'TEST' COLLATE JAVA_NOCASE;").select();
        Assert.assertThat(iterator.hasNext(), Is.is(true));
        Assert.assertThat(iterator.next().getColumnString(0), IsEqual.equalTo("test"));
        iterator.close();
    }

    @After
    public void tearDown() throws Exception {
        mDb.close();
    }

}
