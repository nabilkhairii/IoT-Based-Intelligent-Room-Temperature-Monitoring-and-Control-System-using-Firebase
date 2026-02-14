// 1. Memasukkan Library
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <WiFi.h>
#include "FirebaseESP32.h"

// 2. Definisi Pin dan Tipe Sensor
#define DHTPIN 25       // Pin DATA sensor DHT terhubung ke GPIO 25
#define RELAY_PIN 26    // Pin IN modul relay terhubung ke GPIO 26
#define DHTTYPE DHT11   // Tipe sensor yang digunakan adalah DHT11

// 3. Batas Suhu (Threshold)
const float BATAS_SUHU = 32.0; // Kipas akan menyala jika suhu di atas nilai ini

// 4. Detail Koneksi WiFi Anda
#define WIFI_SSID ""
#define WIFI_PASSWORD ""

// 5. Detail Firebase Anda
#define FIREBASE_HOST ""
// !! GANTI INI DENGAN KUNCI RAHASIA ANDA !!
#define FIREBASE_AUTH "MASUKKAN_DATABASE_SECRET_ANDA_DISINI" 

// 6. Inisialisasi Objek Sensor dan Firebase
DHT dht(DHTPIN, DHTTYPE);
FirebaseData firebaseData;
FirebaseAuth firebaseAuth;
FirebaseConfig firebaseConfig;

// Variabel global untuk status
String statusKipas = "MATI"; // Menyimpan status kipas saat ini

void setup() {
  // Mulai Serial Monitor untuk debugging
  Serial.begin(115200);
  Serial.println("Sistem Kontrol Suhu & Kelembaban Dimulai...");

  // Atur pin relay sebagai OUTPUT
  pinMode(RELAY_PIN, OUTPUT);

  // Inisialisasi sensor DHT
  dht.begin();

  // Atur kondisi awal Kipas = MATI (Asumsi Active-HIGH)
  digitalWrite(RELAY_PIN, LOW);
  Serial.println("Kondisi awal: Kipas MATI.");

  // ---------------------------------
  // KONEKSI WIFI
  // ---------------------------------
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Menghubungkan ke WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Terhubung ke WiFi dengan IP: ");
  Serial.println(WiFi.localIP());

  // ---------------------------------
  // KONEKSI FIREBASE
  // ---------------------------------
  firebaseConfig.host = FIREBASE_HOST;
  firebaseConfig.signer.tokens.legacy_token = FIREBASE_AUTH;
  Firebase.begin(&firebaseConfig, &firebaseAuth);
  Firebase.reconnectWiFi(true);
}

void loop() {
  // Beri jeda 2 detik antar pembacaan (sensor DHT lambat)
  delay(2000);

  // [BARU] Baca Kelembaban (Humidity)
  float h = dht.readHumidity();
  // Baca suhu dalam Celcius
  float t = dht.readTemperature();

  // [MODIFIKASI] Cek apakah pembacaan sensor gagal (untuk suhu ATAU kelembaban)
  if (isnan(t) || isnan(h)) {
    Serial.println("Gagal membaca dari sensor DHT!");
    return; // Keluar dari loop ini jika gagal
  }

  // [MODIFIKASI] Cetak Suhu dan Kelembaban ke Serial Monitor
  Serial.print("Suhu: ");
  Serial.print(t);
  Serial.print(" °C | ");
  Serial.print("Kelembaban: ");
  Serial.print(h);
  Serial.print(" % | ");

  // ---------------------------------
  // LOGIKA KONTROL RELAY (Tetap berdasarkan Suhu)
  // ---------------------------------
  if (t > BATAS_SUHU) {
    digitalWrite(RELAY_PIN, HIGH); // Kirim sinyal LOW (Active-LOW)
    Serial.println("Status Kipas: MENYALA");
    statusKipas = "MENYALA";
  } else {
    digitalWrite(RELAY_PIN, LOW); // Kirim sinyal HIGH (Active-LOW)
    Serial.println("Status Kipas: MATI");
    statusKipas = "MATI";
  }

  // ---------------------------------
  // KIRIM DATA KE FIREBASE
  // ---------------------------------
  if (WiFi.status() == WL_CONNECTED && Firebase.ready()) {
    
    // [MODIFIKASI] Kirim nilai suhu (float) ke path "/Temperature"
    if (Firebase.setFloat(firebaseData, "/Temperature", t)) {
      Serial.println("-> Berhasil kirim Suhu ke Firebase");
    } else {
      Serial.println("-> Gagal kirim Suhu: " + firebaseData.errorReason());
    }

    // [BARU] Kirim nilai kelembaban (float) ke path "/Humidity"
    if (Firebase.setFloat(firebaseData, "/Humidity", h)) {
      Serial.println("-> Berhasil kirim Kelembaban ke Firebase");
    } else {
      Serial.println("-> Gagal kirim Kelembaban: " + firebaseData.errorReason());
    }

    // Kirim status kipas (string) ke path "/StatusKipas"
    if (Firebase.setString(firebaseData, "/StatusKipas", statusKipas)) {
      Serial.println("-> Berhasil kirim Status Kipas ke Firebase");
    } else {
      Serial.println("-> Gagal kirim Status: " + firebaseData.errorReason());
    }
  } else {
    Serial.println("Koneksi WiFi/Firebase terputus, tidak mengirim data.");
  }
}