package sqlite4a;

/**
 * @author Daniel Serdyukov
 */
public class SQLiteException extends RuntimeException {

    public SQLiteException(String message) {
        super(message);
    }

    public SQLiteException(String message, Throwable cause) {
        super(message, cause);
    }

    public SQLiteException(Throwable cause) {
        super(cause);
    }

}
