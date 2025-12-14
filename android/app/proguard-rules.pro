# Add project specific ProGuard rules here.
# You can control the set of applied configuration files using the
# proguardFiles setting in build.gradle.
#
# For more details, see
#   http://developer.android.com/guide/developing/tools/proguard.html

# Preserve all JNI/native method names - critical for JNI callbacks
-keepclasseswithmembernames class * {
    native <methods>;
}

# Preserve AndroidManager and related JNI classes
-keep class com.otclient.** { *; }
-keep class com.derakus.legends.** { *; }

# Keep all Activity, Service, BroadcastReceiver, and other Android components
-keep public class * extends android.app.Activity
-keep public class * extends android.app.Service
-keep public class * extends android.content.BroadcastReceiver
-keep public class * extends android.view.View {
    public <init>(android.content.Context);
    public <init>(android.content.Context, android.util.AttributeSet);
}

# Keep all enums
-keepclassmembers enum * {
    public static **[] values();
    public static ** valueOf(java.lang.String);
}

# Keep native method names
-keepclasseswithmembers class * {
    *** *JNI(...);
}

# Preserve line number information for debugging
-keepattributes SourceFile,LineNumberTable
-renamesourcefileattribute SourceFile

# Keep R classes
-keepclassmembers class **.R$* {
    public static <fields>;
}