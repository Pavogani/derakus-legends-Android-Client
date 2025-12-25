import java.util.Properties

plugins {
    id("com.android.application")
    id("org.jetbrains.kotlin.android")
}

// Load local.properties for keystore credentials
val localProperties = Properties()
val localPropertiesFile = rootProject.file("local.properties")
if (localPropertiesFile.exists()) {
    localProperties.load(localPropertiesFile.inputStream())
}

android {
    namespace = "com.derakus.legends"
    compileSdk = 36

    signingConfigs {
        create("release") {
            storeFile = file("../derakus-legends.keystore")
            storePassword = System.getenv("KEYSTORE_PASSWORD")
                ?: (localProperties.getProperty("KEYSTORE_PASSWORD") ?: "")
            keyAlias = "derakus-legends"
            keyPassword = System.getenv("KEY_PASSWORD")
                ?: (localProperties.getProperty("KEY_PASSWORD") ?: "")
        }
    }

    defaultConfig {
        applicationId = "com.derakus.legends"
        minSdk = 21
        targetSdk = 36
        versionCode = 1
        versionName = "1.0"
        testInstrumentationRunner = "androidx.test.runner.AndroidJUnitRunner"

        ndk {
            // Only build for actual mobile device architectures (excludes x86/x86_64 emulators)
            abiFilters += listOf("arm64-v8a", "armeabi-v7a")
        }

        externalNativeBuild {
            cmake {
                cppFlags += listOf("-std=c++23", "-flto")

                arguments += listOf(
                    "-DVCPKG_TARGET_ANDROID=ON",
                    "-DANDROID_STL=c++_shared"
                )
            }
        }
    }

    externalNativeBuild {
        cmake {
            path = file("../../CMakeLists.txt")
            version = "4.1.0"
        }
    }

    buildTypes {
        getByName("debug") {
            isMinifyEnabled = false
            isShrinkResources = false
            isDebuggable = true
        }
        
        getByName("release") {
            isMinifyEnabled = true
            isShrinkResources = true
            signingConfig = signingConfigs.getByName("release")
            proguardFiles(
                getDefaultProguardFile("proguard-android-optimize.txt"),
                file("proguard-rules.pro")
            )
        }
    }
    
    packaging {
        // Prevent duplicate library errors
        resources {
            excludes += setOf(
                "META-INF/LICENSE",
                "META-INF/LICENSE.txt",
                "META-INF/NOTICE",
                "META-INF/NOTICE.txt"
            )
        }
        jniLibs {
            useLegacyPackaging = false
        }
    }
    
    sourceSets {
        getByName("main") {
            // Ensure assets are included
            assets.srcDirs("src/main/assets")
        }
    }

    compileOptions {
        sourceCompatibility = JavaVersion.toVersion(21)
        targetCompatibility = JavaVersion.toVersion(21)
    }

    kotlinOptions {
        jvmTarget = "21"
    }

    buildFeatures {
        viewBinding = true
        prefab = true
    }

    ndkVersion = "29.0.13599879"
}

dependencies {
    implementation("androidx.core:core-ktx:1.17.0")
    implementation("androidx.appcompat:appcompat:1.7.1")
    implementation("androidx.games:games-activity:1.2.1")
    implementation("com.google.android.material:material:1.13.0")
    implementation("androidx.core:core-splashscreen:1.0.1")
    implementation("androidx.lifecycle:lifecycle-runtime-ktx:2.8.7")
}