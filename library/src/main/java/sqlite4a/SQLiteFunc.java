package sqlite4a;

import android.support.annotation.NonNull;

/**
 * @author Daniel Serdyukov
 */
public interface SQLiteFunc {

    void call(@NonNull SQLiteContext context, @NonNull SQLiteValue[] values);

}
