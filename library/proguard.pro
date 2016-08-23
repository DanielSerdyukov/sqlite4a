-keepattributes *Annotation*,Signature,Exceptions,InnerClasses,EnclosingMethod
-optimizations !method/inlining/*
-keepparameternames

-keep public interface sqlite4a.** { *; }

-keep class sqlite4a.** {
    public static <fields>;
    public <methods>;
    native <methods>;
}