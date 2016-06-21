package sqlite4a;

import android.content.Context;
import android.support.annotation.NonNull;

/**
 * @author Daniel Serdyukov
 */
public class SQLite {

    public static final int OPEN_READONLY = 0x00000001;

    public static final int OPEN_READWRITE = 0x00000002;

    public static final int OPEN_CREATE = 0x00000004;

    public static final int OPEN_URI = 0b1000000;

    public static final int OPEN_NOMUTEX = 0x00008000;

    public static final int OPEN_FULLMUTEX = 0x00010000;

    private static final int BUSY_TIMEOUT_MS = 2500;

    public static void loadLibrary(@NonNull Context context) {
        //ReLinker.loadLibrary(context, "sqlite3_jni");
        Runtime.getRuntime().loadLibrary("sqlite3_jni");
    }

    @NonNull
    public static native String getVersion();

    @NonNull
    public static SQLiteDb open(@NonNull String path) {
        return open(path, OPEN_READWRITE | OPEN_CREATE);
    }

    @NonNull
    public static SQLiteDb open(@NonNull String path, int flags) {
        return open(path, flags, BUSY_TIMEOUT_MS);
    }

    @NonNull
    public static SQLiteDb open(@NonNull String path, int flags, int busyTimeout) {
        return new SQLiteDb(nativeOpenV2(path, flags, busyTimeout));
    }

    private static native long nativeOpenV2(String path, int flags, int busyTimeout);

}
