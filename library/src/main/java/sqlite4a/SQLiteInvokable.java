package sqlite4a;

import android.support.annotation.Keep;
import android.support.annotation.NonNull;

/**
 * @author Daniel Serdyukov
 */
@Keep
class SQLiteInvokable {

    private final SQLiteFunc mFunc;

    SQLiteInvokable(@NonNull SQLiteFunc func) {
        mFunc = func;
    }

    public void invoke(long contextPtr, long[] valuePtrs) {
        final SQLiteValue[] values = new SQLiteValue[valuePtrs.length];
        for (int i = 0; i < valuePtrs.length; ++i) {
            values[i] = new SQLiteValue(valuePtrs[i]);
        }
        mFunc.call(new SQLiteContext(contextPtr), values);
    }

}
