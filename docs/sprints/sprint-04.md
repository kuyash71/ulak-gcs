# Sprint 4 — UI Temel

**Faz:** III  
**Durum:** 🔲 Planlandı  
**Önkoşul:** Sprint 3 tamamlanmış

---

## Hedef

PyQt6 ile ana pencere iskeletini ve temel operasyonel panelleri oluşturmak.
Sprint sonu itibarıyla uygulama arayüzü açılabilmeli ve gerçek telemetri
verisi ekranda görünebilmelidir.

---

## Görevler

### Ana Pencere

- [ ] `src/ui/main_window.py` — `MainWindow(QMainWindow)`
  - Dock tabanlı panel düzeni
  - Menü çubuğu: Dosya (Profile seç/çıkış), Görünüm (panelleri göster/gizle), Yardım
  - Durum çubuğu: bağlantı durumu, aktif profile, araç modu
  - `AppController` sinyallerine abone ol, UI'ı güncelle
  - Uygulama kapatılmadan önce temiz kapatma (bağlantıları kapat)

### Telemetri Paneli

- [ ] `src/ui/panels/telemetry_panel.py` — `TelemetryPanel(QWidget)`
  - Konum (X, Y, Z), hız, yönelim (roll/pitch/yaw) göstergesi
  - Batarya göstergesi (yüzde + voltaj)
  - Araç modu badge'i (renk kodlu)
  - GPS durumu (Geodetic bilgisi varsa)
  - `TelemetryFrame` güncellemelerinde otomatik yenileme (50ms gecikme limiti)

### Misyon Durum Paneli

- [ ] `src/ui/panels/mission_panel.py` — `MissionPanel(QWidget)`
  - Misyon durumu (IDLE/RUNNING/PAUSED/ABORTED/COMPLETED) ile renk kodlu badge
  - İlerleme çubuğu (0.0–1.0)
  - Durum notu (örn. "loop 2/4")
  - `MissionState` güncellemelerinde otomatik yenileme

### Algılama Paneli

- [ ] `src/ui/panels/perception_panel.py` — `PerceptionPanel(QWidget)`
  - Hedef renk ve şekil göstergesi
  - Hizalama vektörü (dx, dy) görselleştirmesi
  - Güven skoru göstergesi
  - Algılama yoksa "Hedef Yok" durumu

### Panik Butonu Widget'ı

- [ ] `src/ui/widgets/panic_button.py` — `PanicButton(QPushButton)`
  - Kırmızı, belirgin, her zaman görünür
  - Çift onay diyaloğu (yanlışlıkla tetiklemeyi önle)
  - Tetiklendiğinde `PanicManager.trigger_panic()` çağır
  - Gönderim sonrası görsel geri bildirim (renk değişimi + log)

### Exception / Safety Olayları Paneli

- [ ] `src/ui/panels/safety_panel.py` — `SafetyPanel(QWidget)`
  - Kaydırılabilir olay listesi (zaman damgası, seviye, kod, mesaj, tavsiye edilen eylem)
  - WARN/ERROR/CRITICAL için renk kodlu satırlar
  - ERROR için onay diyaloğu (UI engelleyici, `T_ack=2s` zaman aşımı göstergesiyle)
  - CRITICAL için tam ekran uyarı overlay + kalıcı banner
  - CRITICAL için opsiyonel sesli uyarı (QSound veya sistem sesi) — Architecture.md §6.3

### Stil ve Tema

- [ ] `assets/themes/dark.qss` — koyu tema stil sayfası
- [ ] `src/ui/theme.py` — tema yükleme ve uygulama yardımcısı

### Testler

- [ ] `tests/unit/test_panic_button.py` — double-confirm mantığını test et
- [ ] `tests/unit/test_safety_panel.py` — olay listesi render mantığını test et

---

## Kabul Kriterleri

- Uygulama `python -m src.platforms.sim.main` ile açılıyor
- Telemetri paneli canlı `TelemetryFrame` verisiyle güncelleniyor
- Panik butonu çift onay olmadan tetiklenemiyor
- CRITICAL olay geldiğinde tam ekran overlay açılıyor
- Tüm paneller dock edilebilir ve gizlenebilir

---

## Notlar

- Tüm UI güncellemeleri PyQt6 sinyal-slot mekanizmasıyla alınır; hiçbir panel doğrudan backend'e erişmez
- Telemetri paneli max 20Hz yenileme; 50ms debounce ile `TelemetryFrame` gelişlerini birleştirir

## Bağımlılıklar

- Sprint 3: AppController sinyalleri, ConnectionState, tüm comms bileşenleri
- Dış: PyQt6
