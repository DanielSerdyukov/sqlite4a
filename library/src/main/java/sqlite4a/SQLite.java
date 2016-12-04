package sqlite4a;

import android.support.annotation.NonNull;

/**
 * @author Daniel Serdyukov
 */
public class SQLite {

    public static final String JNI_LIB = "sqlite3_jni";

    public static final int OPEN_READONLY = 0x00000001;

    public static final int OPEN_READWRITE = 0x00000002;

    public static final int OPEN_CREATE = 0x00000004;

    public static final int OPEN_URI = 0b1000000;

    public static final int OPEN_NOMUTEX = 0x00008000;

    public static final int OPEN_FULLMUTEX = 0x00010000;

    public static native long getLibVersionNumber();

    @NonNull
    public static SQLiteDb open(@NonNull String path, int flags) {
        return new SQLiteDb(nativeOpen(path, flags));
    }

    private static native long nativeOpen(String path, int flags);

}
