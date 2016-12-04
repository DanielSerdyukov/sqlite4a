package sqlite4a;

import android.support.annotation.NonNull;
import android.support.test.InstrumentationRegistry;
import android.support.test.runner.AndroidJUnit4;

import com.getkeepsafe.relinker.ReLinker;

import org.hamcrest.collection.IsArrayContaining;
import org.hamcrest.core.Is;
import org.hamcrest.core.IsCollectionContaining;
import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.ArrayList;
import java.util.List;

/**
 * @author Daniel Serdyukov
 */
@RunWith(AndroidJUnit4.class)
public class SQLiteDbTest {

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
    public void isReadOnly() throws Exception {
        SQLiteDb db = SQLite.open(":memory:", SQLite.OPEN_READONLY);
        Assert.assertThat(db.isReadOnly(), Is.is(true));
        db.close();
    }

    @Test
    public void isReadWrite() throws Exception {
        SQLiteDb db = SQLite.open(":memory:", SQLite.OPEN_READWRITE);
        Assert.assertThat(db.isReadOnly(), Is.is(false));
        db.close();
    }

    @Test
    public void trace() throws Exception {
        final List<String> log = new ArrayList<>();
        mDb.trace(new SQLiteTrace() {
            @Override
            public void call(@NonNull String sql) {
                log.add(sql);
            }
        });
        mDb.exec("CREATE TABLE IF NOT EXISTS test(value TEXT);", null);
        final SQLiteStmt stmt = mDb.prepare("INSERT INTO test VALUES(?);");
        stmt.bindString(1, "this is a test");
        stmt.executeInsert();
        stmt.close();
        Assert.assertThat(log, IsCollectionContaining.hasItems(
                "CREATE TABLE IF NOT EXISTS test(value TEXT);",
                "INSERT INTO test VALUES('this is a test');"
        ));
    }

    @Test
    public void exec() throws Exception {
        mDb.exec("SELECT sqlite_version() AS version;", new SQLiteExec() {
            @Override
            public void call(String[] columns, String[] values) {
                Assert.assertThat(columns, IsArrayContaining.hasItemInArray("version"));
                Assert.assertThat(values, IsArrayContaining.hasItemInArray("3.15.1"));
            }
        });
    }

    @Test
    public void transactionCommit() throws Exception {
        Assert.assertThat(mDb.getAutoCommit(), Is.is(false));
        mDb.exec("BEGIN;", null);
        Assert.assertThat(mDb.getAutoCommit(), Is.is(true));
        mDb.exec("COMMIT;", null);
        Assert.assertThat(mDb.getAutoCommit(), Is.is(false));
    }

    @Test
    public void transactionRollback() throws Exception {
        Assert.assertThat(mDb.getAutoCommit(), Is.is(false));
        mDb.exec("BEGIN;", null);
        Assert.assertThat(mDb.getAutoCommit(), Is.is(true));
        mDb.exec("ROLLBACK;", null);
        Assert.assertThat(mDb.getAutoCommit(), Is.is(false));
    }

    @After
    public void tearDown() throws Exception {
        mDb.close();
    }

}