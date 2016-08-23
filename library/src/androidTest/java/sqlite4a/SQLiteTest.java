package sqlite4a;

import android.support.test.InstrumentationRegistry;
import android.support.test.runner.AndroidJUnit4;

import org.hamcrest.core.Is;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

/**
 * @author Daniel Serdyukov
 */
@RunWith(AndroidJUnit4.class)
public class SQLiteTest {

    @Before
    public void setUp() throws Exception {
        SQLite.loadLibrary(InstrumentationRegistry.getContext());
    }

    @Test
    public void getVersion() throws Exception {
        Assert.assertThat(SQLite.getVersion(), Is.is("3.14.1"));
    }

    @Test(expected = SQLiteException.class)
    public void openWithException() throws Exception {
        SQLite.open("/fake/path/main.db");
    }

    @Test
    public void isReadOnly() throws Exception {
        SQLiteDb db = SQLite.open(":memory:", SQLite.OPEN_READONLY);
        Assert.assertThat(db.isReadOnly(), Is.is(true));
        db.close();

        db = SQLite.open(":memory:");
        Assert.assertThat(db.isReadOnly(), Is.is(false));
        db.close();
    }

}