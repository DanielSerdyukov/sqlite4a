package sqlite4a;

/**
 * @author Daniel Serdyukov
 */
public interface SQLiteExec {

    void call(String[] columns, String[] values);

}
