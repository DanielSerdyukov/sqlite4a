package sqlite4a;

import android.location.Location;
import android.support.annotation.NonNull;
import android.support.test.InstrumentationRegistry;

import com.getkeepsafe.relinker.ReLinker;

import org.hamcrest.core.Is;
import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Test;

import java.util.Arrays;

/**
 * @author Daniel Serdyukov
 */
public class SQLiteFunctionTest {

    private SQLiteDb mDb;

    @BeforeClass
    public static void loadLibrary() {
        ReLinker.loadLibrary(InstrumentationRegistry.getContext(), SQLite.JNI_LIB);
    }

    @Before
    public void setUp() throws Exception {
        mDb = SQLite.open(":memory:", SQLite.OPEN_CREATE | SQLite.OPEN_READWRITE);
        mDb.exec("CREATE TABLE cities(name TEXT, since INTEGER, lat REAL, lng REAL, bytes BLOB);", null);
        mDb.exec("INSERT INTO cities VALUES('Санкт-Петербург', 1703, 59.9388333, 30.315866, x'0103020405');", null);
    }

    @Test
    public void overrideUpper() throws Exception {
        mDb.exec("INSERT INTO cities(rowid, name) VALUES(100, 'санкт');", null);
        mDb.createFunction("upper", 1, new SQLiteFunc() {
            @Override
            public void call(@NonNull SQLiteContext context, @NonNull SQLiteValue[] values) {
                final String value = values[0].stringValue();
                if (value != null) {
                    context.resultString(value.toUpperCase());
                }
            }
        });
        final SQLiteStmt stmt = mDb.prepare("SELECT UPPER(name) FROM cities WHERE rowid = 100;");
        final SQLiteCursor cursor = stmt.executeSelect();
        Assert.assertThat(cursor.step(), Is.is(true));
        Assert.assertThat(cursor.getColumnString(0), Is.is("САНКТ"));
        stmt.close();
    }

    @Test
    public void pow() throws Exception {
        mDb.createFunction("pow", 2, new SQLiteFunc() {
            @Override
            public void call(@NonNull SQLiteContext context, @NonNull SQLiteValue[] values) {
                context.resultLong((long) Math.pow(values[0].longValue(), values[1].longValue()));
            }
        });
        final SQLiteStmt stmt = mDb.prepare("SELECT pow(since, 2) FROM cities;");
        final SQLiteCursor cursor = stmt.executeSelect();
        Assert.assertThat(cursor.step(), Is.is(true));
        Assert.assertThat(cursor.getColumnLong(0), Is.is((long) Math.pow(1703, 2)));
        stmt.close();
    }

    @Test
    public void distanceBetween() throws Exception {
        mDb.createFunction("distanceBetween", 4, new SQLiteFunc() {
            @Override
            public void call(@NonNull SQLiteContext context, @NonNull SQLiteValue[] values) {
                final float[] results = new float[1];
                Location.distanceBetween(values[0].doubleValue(), values[1].doubleValue(),
                        values[2].doubleValue(), values[3].doubleValue(), results);
                context.resultDouble(results[0]);
            }
        });
        final double lat = 59.9957222;
        final double lng = 30.2523343;
        final SQLiteStmt stmt = mDb.prepare("SELECT distanceBetween(lat, lng, ?, ?) FROM cities;");
        stmt.bindDouble(1, lat);
        stmt.bindDouble(2, lng);
        final SQLiteCursor cursor = stmt.executeSelect();
        Assert.assertThat(cursor.step(), Is.is(true));
        final float[] results = new float[1];
        Location.distanceBetween(lat, lng, 59.9388333, 30.315866, results);
        Assert.assertThat(cursor.getColumnDouble(0), Is.is((double) results[0]));
        stmt.close();
    }

    @Test
    public void sort() throws Exception {
        mDb.createFunction("sort", 1, new SQLiteFunc() {
            @Override
            public void call(@NonNull SQLiteContext context, @NonNull SQLiteValue[] values) {
                final byte[] blob = values[0].blobValue();
                Arrays.sort(blob);
                context.resultBlob(blob);
            }
        });
        final SQLiteStmt stmt = mDb.prepare("SELECT sort(bytes) FROM cities;");
        final SQLiteCursor cursor = stmt.executeSelect();
        Assert.assertThat(cursor.step(), Is.is(true));
        Assert.assertThat(cursor.getColumnBlob(0), Is.is(new byte[]{1, 2, 3, 4, 5}));
        stmt.close();
    }

    @After
    public void tearDown() throws Exception {
        mDb.close();
    }

}
