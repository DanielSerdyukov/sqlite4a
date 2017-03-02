/*
 * Copyright 2017 exzogeni.com
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
import android.support.annotation.Nullable;

import java.io.Closeable;

public class SQLiteStmt implements Closeable {

    private final long mStmtPtr;

    SQLiteStmt(long stmtPtr) {
        mStmtPtr = stmtPtr;
    }

    static native void nativeFinalize(long ptr);

    private static native String nativeGetSql(long ptr);

    private static native void nativeBindNull(long ptr, int index);

    private static native void nativeBindLong(long ptr, int index, long value);

    private static native void nativeBindDouble(long ptr, int index, double value);

    private static native void nativeBindString(long ptr, int index, String value);

    private static native void nativeBindBlob(long ptr, int index, byte[] value);

    private static native void nativeClearBindings(long ptr);

    private static native long nativeInsert(long ptr);

    private static native int nativeExecute(long ptr);

    private static native int nativeBusy(long ptr);

    private static native void nativeReset(long ptr);

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

    public long insert() {
        return nativeInsert(mStmtPtr);
    }

    @NonNull
    public SQLiteIterator select() {
        if (nativeBusy(mStmtPtr) > 0) {
            nativeReset(mStmtPtr);
        }
        return new RowIterator(mStmtPtr);
    }

    public int execute() {
        return nativeExecute(mStmtPtr);
    }

    @Override
    public void close() {
        nativeFinalize(mStmtPtr);
    }

}
