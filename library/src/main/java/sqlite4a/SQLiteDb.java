package sqlite4a;

import android.support.annotation.Keep;
import android.support.annotation.NonNull;

import java.io.Closeable;

/**
 * @author Daniel Serdyukov
 */
public class SQLiteDb implements Closeable {

    private final long mDbPtr;

    @Keep
    SQLiteDb(long dbPtr) {
        mDbPtr = dbPtr;
    }

    private static native void nativeCloseV2(long dbPtr);

    private static native boolean nativeIsReadOnly(long dbPtr);

    private static native void nativeTrace(long dbPtr);

    private static native void nativeExec(long dbPtr, String sql);

    private static native double nativeExecForDouble(long dbPtr, String sql);

    private static native int nativeGetAutocommit(long dbPtr);

    private static native long nativePrepareV2(long dbPtr, String sql);

    public boolean isReadOnly() {
        return nativeIsReadOnly(mDbPtr);
    }

    public void enableTracing() {
        nativeTrace(mDbPtr);
    }

    public int getUserVersion() {
        return (int) nativeExecForDouble(mDbPtr, "PRAGMA user_version;");
    }

    public void setUserVersion(int version) {
        nativeExec(mDbPtr, "PRAGMA user_version = " + version + ";");
    }

    public void begin() {
        nativeExec(mDbPtr, "BEGIN;");
    }

    public void commit() {
        nativeExec(mDbPtr, "COMMIT;");
    }

    public void rollback() {
        nativeExec(mDbPtr, "ROLLBACK;");
    }

    public boolean inTransaction() {
        return nativeGetAutocommit(mDbPtr) <= 0;
    }

    public void exec(@NonNull String sql) {
        nativeExec(mDbPtr, sql);
    }

    @NonNull
    public SQLiteStmt prepare(@NonNull String sql) {
        return new SQLiteStmt(nativePrepareV2(mDbPtr, sql));
    }

    @Override
    public void close() {
        nativeCloseV2(mDbPtr);
    }

}
