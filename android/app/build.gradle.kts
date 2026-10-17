plugins {
  id("com.android.application")
  id("org.jetbrains.kotlin.android")
}
android {
  namespace = "dev.darachnid.cwmessenger"
  compileSdk = 35
  defaultConfig {
    applicationId = "dev.darachnid.cwmessenger"
    minSdk = 26
    targetSdk = 35
    versionCode = 1
    versionName = "0.6-dev"
  }
  compileOptions {
    sourceCompatibility = JavaVersion.VERSION_17
    targetCompatibility = JavaVersion.VERSION_17
  }
  kotlinOptions { jvmTarget = "17" }
}
dependencies {
  implementation("io.rebble.pebblekit2:client:1.3.2")
  implementation("org.jetbrains.kotlinx:kotlinx-coroutines-android:1.10.2")
}
