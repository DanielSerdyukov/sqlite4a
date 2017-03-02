/*
 * Copyright 2016-2017 exzogeni.com
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package sqlite4a;

import android.support.annotation.NonNull;

import java.io.Closeable;
import java.util.Comparator;

public class SQLiteDb implements Closeable {

    private final long mDbPtr;

    SQLiteDb(long ptr) {
        mDbPtr = ptr;
    }

    private static native void nativeClose(long ptr);

    private static native void nativeTrace(long ptr, Trace func);

    private static native int nativeIsReadOnly(long ptr);

    private static native int nativeGetAutocommit(long ptr);

    private static native void nativeExec(long ptr, String sql);

    private static native double nativeExecForDouble(long ptr, String sql);

    private static native long nativePrepare(long ptr, String sql);

    private static native void nativeCreateCollation(long ptr, String name, Comparator<String> comparator);

    private static native void nativeCreateFunction(long ptr, String name, int numArgs, JniFunc func);

    public void trace(@NonNull Trace func) {
        nativeTrace(mDbPtr, func);
    }

    public boolean isReadOnly() {
        return nativeIsReadOnly(mDbPtr) != 0;
    }

    public boolean inTransaction() {
        return nativeGetAutocommit(mDbPtr) <= 0;
    }

    public void exec(@NonNull String sql) {
        nativeExec(mDbPtr, sql);
    }

    @NonNull
    public Number execForNumber(@NonNull String sql) {
        return nativeExecForDouble(mDbPtr, sql);
    }

    @NonNull
    public SQLiteStmt prepare(@NonNull String sql) {
        return new SQLiteStmt(nativePrepare(mDbPtr, sql));
    }

    public void createCollation(@NonNull String name, @NonNull Comparator<String> comparator) {
        nativeCreateCollation(mDbPtr, name, comparator);
    }

    public void createFunction(@NonNull String name, int numArgs, @NonNull Func func) {
        nativeCreateFunction(mDbPtr, name, numArgs, new JniFunc(func));
    }

    @Override
    public void close() {
        nativeClose(mDbPtr);
    }

    public interface Trace {
        void trace(String sql);
    }

    public interface Func {
        void call(@NonNull SQLiteContext context, @NonNull SQLiteValue[] values);
    }

}
