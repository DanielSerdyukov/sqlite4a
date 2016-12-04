package sqlite4a;

import android.support.annotation.NonNull;

/**
 * @author Daniel Serdyukov
 */
public interface SQLiteTrace {

    void call(@NonNull String sql);

}
