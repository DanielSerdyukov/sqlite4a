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

/**
 * @author Daniel Serdyukov
 */
@RunWith(AndroidJUnit4.class)
public class SQLiteStmtTest {

    private SQLiteDb mDb;

    @BeforeClass
    public static void loadLibrary() {
        ReLinker.loadLibrary(InstrumentationRegistry.getContext(), SQLite.JNI_LIB);
    }

    @Before
    public void setUp() throws Exception {
        mDb = SQLite.open(":memory:", SQLite.OPEN_CREATE | SQLite.OPEN_READWRITE);
        mDb.exec("CREATE TABLE test(id INTEGER PRIMARY KEY);", null);
    }

    @Test
    public void getSql() throws Exception {
        final SQLiteStmt stmt = mDb.prepare("SELECT sqlite_version();");
        Assert.assertThat(stmt.getSql(), Is.is("SELECT sqlite_version();"));
        stmt.close();
    }

    @Test
    public void executeInsert() throws Exception {
        final SQLiteStmt stmt = mDb.prepare("INSERT INTO test VALUES(?);");
        stmt.bindLong(1, 1000L);
        Assert.assertThat(stmt.executeInsert(), Is.is(1000L));
        for (int i = 1; i <= 10; ++i) {
            stmt.clearBindings();
            stmt.bindNull(1);
            Assert.assertThat(stmt.executeInsert(), Is.is(1000L + i));
        }
        stmt.close();
    }

    @Test
    public void executeUpdateDelete() throws Exception {
        SQLiteStmt stmt = mDb.prepare("INSERT INTO test VALUES(?);");
        for (int i = 0; i < 10; ++i) {
            stmt.executeInsert();
            stmt.clearBindings();
        }
        stmt.close();

        stmt = mDb.prepare("DELETE FROM test WHERE id % 2 = 0;");
        Assert.assertThat(stmt.executeUpdateDelete(), Is.is(5));
        stmt.close();
    }

    @After
    public void tearDown() throws Exception {
        mDb.close();
    }

}