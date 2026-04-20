# ULAK GCS — Proje Roadmap

> **Hedef:** ArduPilot tabanlı insansız hava araçları için tam işlevsel, güvenli ve genişletilebilir bir yer kontrol istasyonu uygulaması geliştirmek.

---

## Durum Özeti

| Katman | Durum | Notlar |
|---|---|---|
| Modeller (`src/models/`) | ✅ Tamamlandı | Dataclass'lar ve enum'lar hazır |
| Dokümantasyon (`docs/`) | ✅ Tamamlandı | Mimari, protokol, ADR'lar |
| Config şemaları | 🔲 Başlanmadı | JSON şemaları boş |
| Uygulama çekirdeği (`src/core/`) | 🔲 Başlanmadı | Stub'lar mevcut |
| İletişim katmanı (`src/comms/`) | 🔲 Başlanmadı | Stub'lar mevcut |
| UI katmanı (`src/ui/`) | 🔲 Başlanmadı | Dizin oluşturulmuş |
| Testler (`tests/`) | ⚠️ Eski C++ | Python testleri yazılacak |
| Dağıtım | 🔲 Başlanmadı | PyInstaller pipeline yok |

---

## Geliştirme Fazları

### Faz I — Temel Altyapı (Sprint 1–2)
Config yükleme, loglama, uygulama çekirdeği ve güvenlik yöneticileri.

**Çıktılar:**
- Çalışır `ConfigLoader` ve profile sistemi
- `AppController`, `ProfileManager`, `PanicManager`, `ExceptionClassifier`
- Tam Python test altyapısı

---

### Faz II — İletişim Katmanı (Sprint 3)
MAVLink/UDP ve ROS2 backend'leriyle telemetri, misyon ve komut bileşenleri.

**Çıktılar:**
- `TelemetryComponent` (MAVLink + ROS2 backends)
- `MissionComponent` + `PerceptionComponent`
- `CommandGateway` ile ACK/REJECT/TIMEOUT döngüsü

---

### Faz III — UI Katmanı (Sprint 4–5)
PyQt6 ana pencere, tüm paneller ve Polisher aracı.

**Çıktılar:**
- Telemetri dashboard, misyon ve algılama panelleri
- Panik butonu widget
- Video stream paneli
- Polisher geliştirici aracı

---

### Faz IV — Entegrasyon ve Dağıtım (Sprint 6)
Uçtan uca test, SITL entegrasyonu, PyInstaller build pipeline.

**Çıktılar:**
- SIM modu (SITL + Gazebo) çalışıyor
- DRONE modu (Pixhawk + RPi) çalışıyor
- Tek dosya çalıştırılabilir binary

---

### Faz V — İleri Özellikler (Sprint 7+)
Replay modu, operatör kontrol listeleri, Lua plugin sistemi.

---

## Milestone Takvimi

| Milestone | Sprint | Hedef |
|---|---|---|
| M1 — Çekirdek Hazır | Sprint 2 sonu | Uygulama başlar, profile yükler, panik çalışır |
| M2 — Comms Hazır | Sprint 3 sonu | Gerçek telemetri alınır, komut gönderilir |
| M3 — MVP UI | Sprint 5 sonu | Tam işlevsel arayüz, SIM ile test edilmiş |
| M4 — Ürün Hazır | Sprint 6 sonu | DRONE modunda çalışan, paketlenmiş binary |

---

## Sprint Dosyaları

- [Sprint 1 — Temel Altyapı](sprints/sprint-01.md)
- [Sprint 2 — Uygulama Çekirdeği](sprints/sprint-02.md)
- [Sprint 3 — İletişim Katmanı](sprints/sprint-03.md)
- [Sprint 4 — UI Temel](sprints/sprint-04.md)
- [Sprint 5 — UI Tamamlama ve Polisher](sprints/sprint-05.md)
- [Sprint 6 — Entegrasyon ve Dağıtım](sprints/sprint-06.md)

---

## Teknik Kısıtlar

- **Panik butonu** her zaman RTL'e sabit; profile tarafından değiştirilemez.
- **Önem seviyeleri** (WARN/ERROR/CRITICAL) sabit; sadece eylem eşlemeleri profile özeldir.
- **XYZ kanonik sözleşme** — tüm telemetri XYZ'ye normalize edilir; UI asla ham çerçeve bilmez.
- **Backend bağımsızlığı** — SIM/DRONE modu değişimi config ile yapılır, kod değişikliği gerekmez.
