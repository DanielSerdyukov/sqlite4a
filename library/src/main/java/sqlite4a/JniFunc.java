package sqlite4a;

import android.support.annotation.Keep;
import android.support.annotation.NonNull;

/**
 * @author Daniel Serdyukov
 */
@Keep
class JniFunc {

    private final SQLiteFunc mFunc;

    JniFunc(@NonNull SQLiteFunc func) {
        mFunc = func;
    }

    void call(long contextPtr, long[] valuePtrs) {
        final SQLiteValue[] values = new SQLiteValue[valuePtrs.length];
        for (int i = 0; i < valuePtrs.length; ++i) {
            values[i] = new SQLiteValue(valuePtrs[i]);
        }
        mFunc.call(new SQLiteContext(contextPtr), values);
    }

}
