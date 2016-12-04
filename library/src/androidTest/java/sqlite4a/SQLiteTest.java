package sqlite4a;

import android.support.test.InstrumentationRegistry;
import android.support.test.runner.AndroidJUnit4;

import com.getkeepsafe.relinker.ReLinker;

import org.hamcrest.core.Is;
import org.junit.Assert;
import org.junit.BeforeClass;
import org.junit.Test;
import org.junit.runner.RunWith;

/**
 * @author Daniel Serdyukov
 */
@RunWith(AndroidJUnit4.class)
public class SQLiteTest {

    @BeforeClass
    public static void loadLibrary() {
        ReLinker.loadLibrary(InstrumentationRegistry.getContext(), SQLite.JNI_LIB);
    }

    @Test
    public void getLibVersionNumber() throws Exception {
        Assert.assertThat(SQLite.getLibVersionNumber(), Is.is(3015001L));
    }

    @Test(expected = SQLiteException.class)
    public void openError() throws Exception {
        SQLite.open("/fake/path/main.db", SQLite.OPEN_CREATE | SQLite.OPEN_READWRITE);
    }

}