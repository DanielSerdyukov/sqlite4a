package sqlite4a;

import android.support.test.InstrumentationRegistry;
import android.support.test.runner.AndroidJUnit4;

import org.hamcrest.core.Is;
import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

/**
 * @author Daniel Serdyukov
 */
@RunWith(AndroidJUnit4.class)
public class SQLiteDbTest {

    private SQLiteDb mDb;

    @Before
    public void setUp() throws Exception {
        SQLite.loadLibrary(InstrumentationRegistry.getContext());
        mDb = SQLite.open(":memory:");
    }

    @Test
    public void testUserVersion() throws Exception {
        Assert.assertThat(mDb.getUserVersion(), Is.is(0));
        mDb.setUserVersion(123);
        Assert.assertThat(mDb.getUserVersion(), Is.is(123));
    }

    @Test
    public void exec() throws Exception {
        mDb.enableTracing();
        mDb.exec("SELECT sqlite_version();");
    }

    @Test
    public void testBeginCommit() throws Exception {
        Assert.assertThat(mDb.inTransaction(), Is.is(false));
        mDb.begin();
        Assert.assertThat(mDb.inTransaction(), Is.is(true));
        mDb.commit();
        Assert.assertThat(mDb.inTransaction(), Is.is(false));
    }

    @Test
    public void testBeginRollback() throws Exception {
        Assert.assertThat(mDb.inTransaction(), Is.is(false));
        mDb.begin();
        Assert.assertThat(mDb.inTransaction(), Is.is(true));
        mDb.rollback();
        Assert.assertThat(mDb.inTransaction(), Is.is(false));
    }

    @After
    public void tearDown() throws Exception {
        mDb.close();
    }

}