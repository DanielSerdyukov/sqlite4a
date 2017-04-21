-keepattributes Signature,Exceptions,InnerClasses,EnclosingMethod,*Annotation*

-keep public interface alchemy.** { *; }

-keep class sqlite4a.JniFunc {
    void call(long, long[]);
}

-keep public class sqlite4a.** {
    public static <fields>;
    public <methods>;
}