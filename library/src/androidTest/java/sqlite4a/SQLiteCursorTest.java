package sqlite4a;

import android.support.test.InstrumentationRegistry;
import android.support.test.runner.AndroidJUnit4;

import com.getkeepsafe.relinker.ReLinker;

import org.hamcrest.core.Is;
import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.security.SecureRandom;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Random;
import java.util.UUID;

/**
 * @author Daniel Serdyukov
 */
@RunWith(AndroidJUnit4.class)
public class SQLiteCursorTest {

    private final List<Entry> mEntries = new ArrayList<>();

    private SQLiteDb mDb;

    @BeforeClass
    public static void loadLibrary() {
        ReLinker.loadLibrary(InstrumentationRegistry.getContext(), SQLite.JNI_LIB);
    }

    @Before
    public void setUp() throws Exception {
        final Random random = new SecureRandom();
        mDb = SQLite.open(":memory:", SQLite.OPEN_CREATE | SQLite.OPEN_READWRITE);
        mDb.exec("CREATE TABLE test(id INTEGER PRIMARY KEY, a INTEGER, b REAL, c TEXT, d BLOB);", null);
        final SQLiteStmt stmt = mDb.prepare("INSERT INTO test VALUES(?, ?, ?, ?, ?);");
        mDb.exec("BEGIN;", null);
        for (int i = 0; i < 10; ++i) {
            final Entry entry = Entry.create(random);
            stmt.bindNull(1);
            stmt.bindLong(2, entry.mA);
            stmt.bindDouble(3, entry.mB);
            stmt.bindString(4, entry.mC);
            stmt.bindBlob(5, entry.mD);
            entry.mId = stmt.executeInsert();
            mEntries.add(entry);
            stmt.clearBindings();
        }
        mDb.exec("COMMIT;", null);
        stmt.close();
    }

    @Test
    public void testSelect() throws Exception {
        final SQLiteStmt stmt = mDb.prepare("SELECT * FROM test;");
        final SQLiteCursor cursor = stmt.executeSelect();
        for (int i = 0; i < 10; ++i) {
            Assert.assertThat(cursor.step(), Is.is(true));
            Assert.assertThat(Entry.create(cursor), Is.is(mEntries.get(i)));
        }
    }

    @Test
    public void testGetColumnNames() throws Exception {
        final SQLiteStmt stmt = mDb.prepare("SELECT * FROM test;");
        final SQLiteCursor cursor = stmt.executeSelect();
        Assert.assertThat(cursor.getColumnCount(), Is.is(5));
        Assert.assertThat(cursor.getColumnName(0), Is.is("id"));
        Assert.assertThat(cursor.getColumnName(1), Is.is("a"));
        Assert.assertThat(cursor.getColumnName(2), Is.is("b"));
        Assert.assertThat(cursor.getColumnName(3), Is.is("c"));
        Assert.assertThat(cursor.getColumnName(4), Is.is("d"));
    }

    @After
    public void tearDown() throws Exception {
        mEntries.clear();
        mDb.close();
    }

    static class Entry {

        private long mId;

        private int mA;

        private double mB;

        private String mC;

        private byte[] mD;

        static Entry create(Random random) {
            final Entry entry = new Entry();
            entry.mA = random.nextInt();
            entry.mB = random.nextDouble();
            entry.mC = UUID.randomUUID().toString();
            entry.mD = new byte[8];
            random.nextBytes(entry.mD);
            return entry;
        }

        static Entry create(SQLiteCursor cursor) {
            final Entry entry = new Entry();
            entry.mId = cursor.getColumnLong(0);
            entry.mA = (int) cursor.getColumnLong(1);
            entry.mB = cursor.getColumnDouble(2);
            entry.mC = cursor.getColumnString(3);
            entry.mD = cursor.getColumnBlob(4);
            return entry;
        }

        @Override
        public boolean equals(Object other) {
            if (this == other) {
                return true;
            }
            if (other == null || getClass() != other.getClass()) {
                return false;
            }
            final Entry entry = (Entry) other;
            return mId == entry.mId
                    && mA == entry.mA
                    && Double.compare(entry.mB, mB) == 0
                    && (mC != null ? mC.equals(entry.mC) : entry.mC == null
                    && Arrays.equals(mD, entry.mD));
        }


        @Override
        public int hashCode() {
            int result;
            long temp;
            result = (int) (mId ^ (mId >>> 32));
            result = 31 * result + mA;
            temp = Double.doubleToLongBits(mB);
            result = 31 * result + (int) (temp ^ (temp >>> 32));
            result = 31 * result + (mC != null ? mC.hashCode() : 0);
            result = 31 * result + Arrays.hashCode(mD);
            return result;
        }

        @Override
        public String toString() {
            return "Entry{" +
                    "mId=" + mId +
                    ", mA=" + mA +
                    ", mB=" + mB +
                    ", mC='" + mC + '\'' +
                    ", mD=" + Arrays.toString(mD) +
                    '}';
        }

    }

}