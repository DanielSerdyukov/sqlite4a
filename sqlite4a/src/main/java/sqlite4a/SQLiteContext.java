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

/**
 * @author Daniel Serdyukov
 */
public class SQLiteContext {

    private final long mContextPtr;

    SQLiteContext(long contextPtr) {
        mContextPtr = contextPtr;
    }

    private static native void nativeResultNull(long contextPtr);

    private static native void nativeResultLong(long contextPtr, long result);

    private static native void nativeResultText(long contextPtr, String result);

    private static native void nativeResultDouble(long contextPtr, double result);

    private static native void nativeResultBlob(long contextPtr, byte[] result);

    public void resultNull() {
        nativeResultNull(mContextPtr);
    }

    public void resultLong(long result) {
        nativeResultLong(mContextPtr, result);
    }

    public void resultString(String result) {
        if (result != null) {
            nativeResultText(mContextPtr, result);
        }
    }

    public void resultDouble(double result) {
        nativeResultDouble(mContextPtr, result);
    }

    public void resultBlob(byte[] result) {
        if (result != null) {
            nativeResultBlob(mContextPtr, result);
        }
    }

}
