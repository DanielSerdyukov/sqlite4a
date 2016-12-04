package sqlite4a;

import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import java.io.Closeable;
import java.util.Comparator;

/**
 * @author Daniel Serdyukov
 */
public class SQLiteDb implements Closeable {

    private final long mPtr;

    SQLiteDb(long ptr) {
        mPtr = ptr;
    }

    private static native void nativeClose(long ptr);

    private static native int nativeIsReadOnly(long ptr);

    private static native void nativeTrace(long ptr, SQLiteTrace func);

    private static native void nativeExec(long ptr, String sql, SQLiteExec func);

    private static native double nativeExecForDouble(long ptr, String sql);

    private static native int nativeGetAutocommit(long ptr);

    private static native long nativePrepare(long ptr, String sql);

    private static native void nativeCreateCollation(long ptr, String name, Comparator<String> comparator);

    private static native void nativeCreateFunction(long ptr, String name, int numArgs, JniFunc func);

    public boolean isReadOnly() {
        return nativeIsReadOnly(mPtr) != 0;
    }

    public boolean getAutoCommit() {
        return nativeGetAutocommit(mPtr) <= 0;
    }

    public void trace(@Nullable SQLiteTrace func) {
        nativeTrace(mPtr, func);
    }

    public void exec(@NonNull String sql, @Nullable SQLiteExec func) {
        nativeExec(mPtr, sql, func);
    }

    @NonNull
    public Number execForNumber(@NonNull String sql) {
        return nativeExecForDouble(mPtr, sql);
    }

    @NonNull
    public SQLiteStmt prepare(@NonNull String sql) {
        return new SQLiteStmt(nativePrepare(mPtr, sql));
    }

    public void createCollation(@NonNull String name, @NonNull Comparator<String> comparator) {
        nativeCreateCollation(mPtr, name, comparator);
    }

    public void createFunction(@NonNull String name, int numArgs, @NonNull SQLiteFunc func) {
        nativeCreateFunction(mPtr, name, numArgs, new JniFunc(func));
    }

    @Override
    public void close() {
        nativeClose(mPtr);
    }

}
