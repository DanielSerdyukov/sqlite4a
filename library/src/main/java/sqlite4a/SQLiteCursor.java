package sqlite4a;

import android.support.annotation.Keep;
import android.support.annotation.NonNull;

/**
 * @author Daniel Serdyukov
 */
public class SQLiteCursor {

    private static final int SQLITE_ROW = 100;

    private final long mStmtPtr;

    @Keep
    SQLiteCursor(long stmtPtr) {
        mStmtPtr = stmtPtr;
    }

    private static native int nativeStep(long stmtPtr);

    private static native long nativeGetColumnLong(long stmtPtr, int index);

    private static native double nativeGetColumnDouble(long stmtPtr, int index);

    private static native String nativeGetColumnString(long stmtPtr, int index);

    private static native byte[] nativeGetColumnBlob(long stmtPtr, int index);

    private static native int nativeGetColumnCount(long stmtPtr);

    private static native String nativeGetColumnName(long stmtPtr, int index);

    public boolean step() {
        return nativeStep(mStmtPtr) == SQLITE_ROW;
    }

    public long getColumnLong(int index) {
        return nativeGetColumnLong(mStmtPtr, index);
    }

    public double getColumnDouble(int index) {
        return nativeGetColumnDouble(mStmtPtr, index);
    }

    public String getColumnString(int index) {
        return nativeGetColumnString(mStmtPtr, index);
    }

    public byte[] getColumnBlob(int index) {
        return nativeGetColumnBlob(mStmtPtr, index);
    }

    public int getColumnCount() {
        return nativeGetColumnCount(mStmtPtr);
    }

    @NonNull
    public String getColumnName(int index) {
        return nativeGetColumnName(mStmtPtr, index);
    }

}
