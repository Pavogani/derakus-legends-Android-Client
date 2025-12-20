# Add project specific ProGuard rules here.
# You can control the set of applied configuration files using the
# proguardFiles setting in build.gradle.
#
# For more details, see
#   http://developer.android.com/guide/developing/tools/proguard.html

# ===== Android Games SDK (GameActivity) =====
# Keep all GameActivity classes - required for native game loop
-keep class com.google.androidgamesdk.** { *; }
-keep class androidx.games.** { *; }

# ===== AndroidX Core (required by GameActivity JNI) =====
# GameActivity native code calls getWindowInsets() returning androidx.core.graphics.Insets
-keep class androidx.core.graphics.Insets { *; }
-keep class androidx.core.view.** { *; }
-keep class androidx.core.** { *; }

# ===== Application Classes =====
# Preserve all app classes with their members (JNI callbacks)
-keep class com.derakus.legends.** { *; }
-keepclassmembers class com.derakus.legends.** { *; }

# ===== JNI/Native Methods =====
# Preserve all native method names - critical for JNI
-keepclasseswithmembernames class * {
    native <methods>;
}

# Keep classes that have native methods
-keepclasseswithmembers class * {
    native <methods>;
}

# ===== Android Components =====
# Keep all Activity classes (including GameActivity subclasses)
-keep public class * extends android.app.Activity { *; }
-keep public class * extends com.google.androidgamesdk.GameActivity { *; }
-keep public class * extends android.app.Service
-keep public class * extends android.content.BroadcastReceiver

# Keep custom View classes with all constructors
-keep public class * extends android.view.View {
    public <init>(android.content.Context);
    public <init>(android.content.Context, android.util.AttributeSet);
    public <init>(android.content.Context, android.util.AttributeSet, int);
    public <init>(android.content.Context, android.util.AttributeSet, int, int);
}

# ===== InputConnection for keyboard handling =====
-keep class * extends android.view.inputmethod.BaseInputConnection { *; }
-keep class * extends android.view.inputmethod.InputConnection { *; }

# ===== Enums =====
-keepclassmembers enum * {
    public static **[] values();
    public static ** valueOf(java.lang.String);
}

# ===== Debugging =====
# Preserve line number information for crash reports
-keepattributes SourceFile,LineNumberTable
-renamesourcefileattribute SourceFile

# Keep exception names
-keepattributes Exceptions

# ===== R classes =====
-keepclassmembers class **.R$* {
    public static <fields>;
}

# ===== Prevent stripping of referenced but unused code =====
-dontwarn com.google.androidgamesdk.**
-dontwarn androidx.games.**
