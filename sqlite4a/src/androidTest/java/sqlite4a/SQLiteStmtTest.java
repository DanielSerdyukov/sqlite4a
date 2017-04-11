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

import org.hamcrest.Matchers;
import org.hamcrest.core.Is;
import org.hamcrest.core.IsEqual;
import org.hamcrest.core.IsNot;
import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.security.SecureRandom;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Iterator;
import java.util.List;
import java.util.Random;
import java.util.UUID;

@RunWith(AndroidJUnit4.class)
public class SQLiteStmtTest {

    private SQLiteDb mDb;

    private List<Entry> mEntries;

    @BeforeClass
    public static void loadLibrary() {
        ReLinker.loadLibrary(InstrumentationRegistry.getContext(), SQLite.JNI_LIB);
    }

    @Before
    public void setUp() throws Exception {
        mDb = SQLite.open(":memory:", SQLite.OPEN_READWRITE | SQLite.OPEN_CREATE);
        mDb.exec("CREATE TABLE test(_id INTEGER PRIMARY KEY, " +
                "int INTEGER, " +
                "text TEXT, " +
                "real REAL, " +
                "blob BLOB);");
        mEntries = new ArrayList<>();
        final Random random = new SecureRandom();
        final SQLiteStmt stmt = mDb.prepare("INSERT INTO test(_id, int, text, real, blob) VALUES(?, ?, ?, ?, ?);");
        final long[] ids = new long[10];
        for (int i = 0; i < ids.length; ++i) {
            final Entry entry = Entry.generate(random);
            stmt.bindLong(2, entry.mInt);
            stmt.bindString(3, entry.mText);
            stmt.bindDouble(4, entry.mReal);
            stmt.bindBlob(5, entry.mBlob);
            ids[i] = stmt.insert();
            stmt.clearBindings();
            mEntries.add(entry);
        }
        stmt.close();
        Assert.assertThat(ids, IsEqual.equalTo(new long[]{
                1, 2, 3, 4, 5, 6, 7, 8, 9, 10
        }));
    }

    @Test
    public void select() throws Exception {
        final SQLiteIterator actual = mDb.prepare("SELECT * FROM test;").select();
        final Iterator<Entry> expected = mEntries.iterator();
        while (actual.hasNext() && expected.hasNext()) {
            final Entry entry = Entry.map(actual.next());
            Assert.assertThat(entry, IsEqual.equalTo(expected.next()));
            Assert.assertThat(entry.mId, Matchers.greaterThan(0L));
        }
        Assert.assertThat(actual.hasNext(), Is.is(false));
        Assert.assertThat(expected.hasNext(), Is.is(false));
        actual.close();
    }

    @Test
    public void execute_delete() throws Exception {
        final Entry removed = mEntries.remove(5);
        final SQLiteStmt stmt = mDb.prepare("DELETE FROM test WHERE text = ?;");
        stmt.bindString(1, removed.mText);
        Assert.assertThat(stmt.execute(), Is.is(1));
        stmt.close();
        final SQLiteIterator actual = mDb.prepare("SELECT * FROM test;").select();
        while (actual.hasNext()) {
            Assert.assertThat(Entry.map(actual.next()), IsNot.not(IsEqual.equalTo(removed)));
        }
        actual.close();
    }

    @After
    public void tearDown() throws Exception {
        mDb.close();
    }

    private static class Entry {

        long mId;

        int mInt;

        String mText;

        double mReal;

        byte[] mBlob;

        static Entry generate(Random random) {
            final Entry entry = new Entry();
            entry.mInt = random.nextInt();
            entry.mText = UUID.randomUUID().toString();
            entry.mReal = random.nextDouble();
            entry.mBlob = new byte[16];
            random.nextBytes(entry.mBlob);
            return entry;
        }

        static Entry map(SQLiteRow row) {
            final Entry entry = new Entry();
            entry.mId = row.getColumnLong(0);
            entry.mInt = (int) row.getColumnLong(1);
            entry.mText = row.getColumnString(2);
            entry.mReal = row.getColumnDouble(3);
            entry.mBlob = row.getColumnBlob(4);
            return entry;
        }

        @Override
        public boolean equals(Object o) {
            if (this == o) {
                return true;
            }
            if (o == null || getClass() != o.getClass()) {
                return false;
            }
            Entry entry = (Entry) o;
            return mInt == entry.mInt
                    && Double.compare(entry.mReal, mReal) == 0
                    && (mText != null ? mText.equals(entry.mText) : entry.mText == null
                    && Arrays.equals(mBlob, entry.mBlob));

        }

        @Override
        public int hashCode() {
            int result;
            long temp;
            result = mInt;
            result = 31 * result + (mText != null ? mText.hashCode() : 0);
            temp = Double.doubleToLongBits(mReal);
            result = 31 * result + (int) (temp ^ (temp >>> 32));
            result = 31 * result + Arrays.hashCode(mBlob);
            return result;
        }

    }

}