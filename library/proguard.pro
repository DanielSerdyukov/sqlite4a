-keepattributes Signature,Exceptions,InnerClasses,EnclosingMethod,*Annotation*
-optimizations !method/inlining/*,!code/allocation/variable
-keepparameternames

-keep public interface sqlite4a.** { *; }

-keep public class sqlite4a.** {
    public static <fields>;
    public <methods>;
}