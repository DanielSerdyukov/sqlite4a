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

/**
 * @author Daniel Serdyukov
 */
public class SQLiteValue {

    private final long mValuePtr;

    SQLiteValue(long valuePtr) {
        mValuePtr = valuePtr;
    }

    static native long nativeLongValue(long valuePtr);

    static native String nativeStringValue(long valuePtr);

    static native double nativeDoubleValue(long valuePtr);

    static native byte[] nativeBlobValue(long valuePtr);

    public long longValue() {
        return nativeLongValue(mValuePtr);
    }

    public String stringValue() {
        return nativeStringValue(mValuePtr);
    }

    public double doubleValue() {
        return nativeDoubleValue(mValuePtr);
    }

    public byte[] blobValue() {
        return nativeBlobValue(mValuePtr);
    }

}
