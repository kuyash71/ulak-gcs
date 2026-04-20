# Sprint 2 — Uygulama Çekirdeği

**Faz:** I  
**Durum:** 🔲 Planlandı  
**Önkoşul:** Sprint 1 tamamlanmış

---

## Hedef

Uygulamanın merkezi koordinasyon ve güvenlik katmanını oluşturmak.
Sprint sonu itibarıyla uygulama ayağa kalkabilmeli, profile yükleyebilmeli
ve panik komutu verebilmelidir.

---

## Görevler

### ExceptionClassifier

- [ ] `src/core/exception_classifier.py`
  - Aktif profile'dan event code → severity eşlemesini yükle
  - `classify(event_code) -> Severity` metodu
  - Bilinmeyen event code için `WARN` fallback
  - `docs/spec/exception-handling.md` kontratına uyum

### ProfileManager

- [ ] `src/core/profile_manager.py`
  - Mevcut profile'ı yükle ve sakla
  - `load_profile(name)` — runtime profile değişimi
  - `default` profile'ın silinmesini/kaldırılmasını engelle
  - Profile değişimini `ExceptionClassifier`'a bildir

### PanicManager

- [ ] `src/core/panic_manager.py`
  - Profile kontrolünün dışında, sabit RTL davranışı
  - `trigger_panic()` — MAVLink SET_MODE (RTL) gönder
  - Fire-and-forget: panik anında bağlantı durumu ikincil öneme sahip
  - Her tetiklemeyi zaman damgasıyla logla (audit trail)
  - `docs/spec/panic-button.md` ve `docs/adr/003-panic-button-system.md` ile uyum

### Bağlantı Durum Makinesi

- [ ] `src/core/connection_state.py` — `ConnectionState` enum
  - `DISCONNECTED`, `CONNECTING`, `CONNECTED`, `MISSION_IDLE`, `MISSION_RUNNING`, `MISSION_PAUSED`, `MISSION_ABORTED`, `MISSION_COMPLETED`
  - Architecture.md §5 durum makinesini yansıtır
  - Geçersiz geçişlerde `ValueError` fırlatır

### AppController

- [ ] `src/core/app_controller.py`
  - Merkezi durum toplayıcı (singleton)
  - `ConnectionState` FSM'ini yönet ve geçişleri logla
  - En son `TelemetryFrame`, `MissionState`, `PerceptionTarget`, `SafetyEvent` tut
  - `ProfileManager`, `PanicManager`, `ExceptionClassifier`'ı başlat
  - PyQt6 sinyalleri ile durum güncellemelerini yayınla:
    - `telemetry_updated(TelemetryFrame)`
    - `mission_updated(MissionState)`
    - `perception_updated(PerceptionTarget)`
    - `safety_event(SafetyEvent)`
    - `connection_state_changed(ConnectionState)`
  - `src/core/__init__.py` — public API dışa aktarımları

### Platform Giriş Noktaları

- [ ] `src/platforms/sim/main.py` — SIM modu başlangıcı
  - `ConfigLoader` ile config yükle
  - `AppController` başlat
  - Log: başlangıç modu, aktif profile, endpoint'ler
- [ ] `src/platforms/drone/main.py` — DRONE modu başlangıcı
  - Aynı yapı, farklı backend seçimi

### Testler

- [ ] `tests/unit/test_exception_classifier.py`
- [ ] `tests/unit/test_profile_manager.py`
- [ ] `tests/unit/test_panic_manager.py`
- [ ] `tests/unit/test_app_controller.py`
- [ ] `tests/unit/test_connection_state.py` — geçersiz FSM geçişleri test et

---

## Kabul Kriterleri

- `python -m src.platforms.sim.main` — crash olmadan başlıyor, `ConnectionState.DISCONNECTED` ile başlıyor
- Profile değişimi çalışma zamanında `ExceptionClassifier`'ı güncelliyor
- `panic_manager.trigger_panic()` MAVLink SET_MODE (RTL) gönderiyor (veya bağlantı yoksa loglıyor)
- `default` profile silinmeye çalışıldığında `ValueError` fırlatıyor
- Geçersiz `ConnectionState` geçişi (örn. DISCONNECTED→MISSION_RUNNING) `ValueError` fırlatıyor

---

## Bağımlılıklar

- Sprint 1: ConfigLoader, Logger, JSON yardımcı araçları
