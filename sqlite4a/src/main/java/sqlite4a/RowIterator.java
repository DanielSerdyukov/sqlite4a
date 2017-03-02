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

import java.util.NoSuchElementException;

class RowIterator implements SQLiteIterator, SQLiteRow {

    private static final int SQLITE_ROW = 100;

    private static final int SQLITE_DONE = 101;

    private final long mStmtPtr;

    private boolean mHasNext;

    private boolean mDone;

    RowIterator(long stmtPtr) {
        mStmtPtr = stmtPtr;
    }

    private static native int nativeStep(long ptr);

    private static native int nativeGetColumnCount(long ptr);

    private static native String nativeGetColumnName(long ptr, int index);

    private static native long nativeGetColumnLong(long ptr, int index);

    private static native double nativeGetColumnDouble(long ptr, int index);

    private static native String nativeGetColumnString(long ptr, int index);

    private static native byte[] nativeGetColumnBlob(long ptr, int index);

    @Override
    public boolean hasNext() {
        if (!mDone) {
            final int step = nativeStep(mStmtPtr);
            mDone = step == SQLITE_DONE;
            mHasNext = step == SQLITE_ROW;
        }
        return !mDone && mHasNext;
    }

    @Override
    public SQLiteRow next() {
        if (mHasNext) {
            mHasNext = false;
            return this;
        }
        throw new NoSuchElementException();
    }

    @Override
    public void remove() {
        throw new UnsupportedOperationException();
    }

    @Override
    public void close() {
        SQLiteStmt.nativeFinalize(mStmtPtr);
    }

    public int getColumnCount() {
        return nativeGetColumnCount(mStmtPtr);
    }

    @NonNull
    public String getColumnName(int index) {
        return nativeGetColumnName(mStmtPtr, index);
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

}
