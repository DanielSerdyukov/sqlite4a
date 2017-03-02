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

import android.support.annotation.IntDef;
import android.support.annotation.NonNull;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

public class SQLite {

    public static final String JNI_LIB = "sqlite3_jni";

    public static final int OPEN_READONLY = 0x00000001;

    public static final int OPEN_READWRITE = 0x00000002;

    public static final int OPEN_CREATE = 0x00000004;

    public static final int OPEN_URI = 0b1000000;

    public static final int OPEN_NOMUTEX = 0x00008000;

    public static final int OPEN_FULLMUTEX = 0x00010000;

    public static native long getLibVersion();

    @NonNull
    public static SQLiteDb open(@NonNull String path, @OpenFlags int flags) {
        return new SQLiteDb(nativeOpen(path, flags));
    }

    private static native long nativeOpen(String path, int flags);

    @Retention(RetentionPolicy.SOURCE)
    @IntDef(value = {
            OPEN_READONLY,
            OPEN_READWRITE,
            OPEN_CREATE,
            OPEN_URI,
            OPEN_NOMUTEX,
            OPEN_FULLMUTEX
    }, flag = true)
    public @interface OpenFlags {

    }

}
