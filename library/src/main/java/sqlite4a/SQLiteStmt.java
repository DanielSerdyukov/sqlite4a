package sqlite4a;

import android.support.annotation.Keep;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import java.io.Closeable;

/**
 * @author Daniel Serdyukov
 */
public class SQLiteStmt implements Closeable {

    private final long mStmtPtr;

    @Keep
    SQLiteStmt(long stmtPtr) {
        mStmtPtr = stmtPtr;
    }

    private static native String nativeGetSql(long stmtPtr);

    private static native String nativeGetExpandedSql(long stmtPtr);

    private static native void nativeBindNull(long stmtPtr, int index);

    private static native void nativeBindLong(long stmtPtr, int index, long value);

    private static native void nativeBindDouble(long stmtPtr, int index, double value);

    private static native void nativeBindString(long stmtPtr, int index, String value);

    private static native void nativeBindBlob(long stmtPtr, int index, byte[] value);

    private static native void nativeClearBindings(long stmtPtr);

    private static native long nativeExecuteInsert(long stmtPtr);

    private static native int nativeExecuteUpdateDelete(long stmtPtr);

    private static native void nativeFinalize(long stmtPtr);

    @NonNull
    public String getSql() {
        return nativeGetSql(mStmtPtr);
    }

    @NonNull
    public String getExpandedSql() {
        return nativeGetExpandedSql(mStmtPtr);
    }

    public void bindNull(int index) {
        nativeBindNull(mStmtPtr, index);
    }

    public void bindLong(int index, long value) {
        nativeBindLong(mStmtPtr, index, value);
    }

    public void bindDouble(int index, double value) {
        nativeBindDouble(mStmtPtr, index, value);
    }

    public void bindString(int index, @Nullable String value) {
        if (value != null) {
            nativeBindString(mStmtPtr, index, value);
        }
    }

    public void bindBlob(int index, @Nullable byte[] value) {
        if (value != null) {
            nativeBindBlob(mStmtPtr, index, value);
        }
    }

    public void clearBindings() {
        nativeClearBindings(mStmtPtr);
    }

    public long executeInsert() {
        return nativeExecuteInsert(mStmtPtr);
    }

    @NonNull
    public SQLiteCursor executeSelect() {
        return new SQLiteCursor(mStmtPtr);
    }

    public int executeUpdateDelete() {
        return nativeExecuteUpdateDelete(mStmtPtr);
    }

    @Override
    public void close() {
        nativeFinalize(mStmtPtr);
    }

}
