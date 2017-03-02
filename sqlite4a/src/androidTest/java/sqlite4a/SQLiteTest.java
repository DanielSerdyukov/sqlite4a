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

import android.support.test.InstrumentationRegistry;
import android.support.test.runner.AndroidJUnit4;

import com.getkeepsafe.relinker.ReLinker;

import org.hamcrest.core.Is;
import org.junit.Assert;
import org.junit.BeforeClass;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class SQLiteTest {

    @BeforeClass
    public static void loadLibrary() {
        ReLinker.loadLibrary(InstrumentationRegistry.getContext(), SQLite.JNI_LIB);
    }

    @Test
    public void sqlite_libversion() throws Exception {
        Assert.assertThat(SQLite.getLibVersion(), Is.is(3017000L));
    }

}
