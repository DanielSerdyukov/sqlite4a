package sqlite4a;

import android.support.annotation.Keep;

/**
 * @author Daniel Serdyukov
 */
public class SQLiteContext {

    private final long mContextPtr;

    @Keep
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
