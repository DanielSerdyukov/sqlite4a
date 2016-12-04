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

import java.util.Comparator;

/**
 * @author Daniel Serdyukov
 */
@RunWith(AndroidJUnit4.class)
public class SQLiteCollationTest {

    private SQLiteDb mDb;

    @BeforeClass
    public static void loadLibrary() {
        ReLinker.loadLibrary(InstrumentationRegistry.getContext(), SQLite.JNI_LIB);
    }

    @Before
    public void setUp() throws Exception {
        mDb = SQLite.open(":memory:", SQLite.OPEN_CREATE | SQLite.OPEN_READWRITE);
    }

    @Test
    public void tableCollation() throws Exception {
        mDb.createCollation("UTF8_NOCASE", new Comparator<String>() {
            @Override
            public int compare(String lhs, String rhs) {
                return lhs.compareToIgnoreCase(rhs);
            }
        });
        mDb.exec("CREATE TABLE test(foo TEXT COLLATE UTF8_NOCASE);", null);
        mDb.exec("INSERT INTO test VALUES('TEST');", null);
        mDb.exec("INSERT INTO test VALUES('ТЕСТ');", null);

        final SQLiteStmt stmt = mDb.prepare("SELECT * FROM test WHERE foo = ?;");
        stmt.bindString(1, "тест");
        final SQLiteCursor cursor = stmt.executeSelect();
        Assert.assertThat(cursor.step(), Is.is(true));
        Assert.assertThat(cursor.getColumnString(0), Is.is("ТЕСТ"));
        stmt.close();
    }

    @Test
    public void queryCollation() throws Exception {
        mDb.createCollation("UTF8_NOCASE", new Comparator<String>() {
            @Override
            public int compare(String lhs, String rhs) {
                return lhs.compareToIgnoreCase(rhs);
            }
        });
        mDb.exec("CREATE TABLE test(foo TEXT);", null);
        mDb.exec("INSERT INTO test VALUES('TEST');", null);
        mDb.exec("INSERT INTO test VALUES('ТЕСТ');", null);

        final SQLiteStmt stmt = mDb.prepare("SELECT * FROM test WHERE foo = ? COLLATE UTF8_NOCASE;");
        stmt.bindString(1, "тест");
        final SQLiteCursor cursor = stmt.executeSelect();
        Assert.assertThat(cursor.step(), Is.is(true));
        Assert.assertThat(cursor.getColumnString(0), Is.is("ТЕСТ"));
        stmt.close();
    }

    @After
    public void tearDown() throws Exception {
        mDb.close();
    }

}
