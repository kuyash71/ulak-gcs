# Sprint 1 — Temel Altyapı

**Faz:** I  
**Durum:** 🔲 Planlandı  

---

## Hedef

Uygulamanın geri kalanının üzerine inşa edileceği sağlam temeli kurmak:
config yükleme, loglama altyapısı ve Python test çerçevesi.

---

## Görevler

### Config Sistemi

- [ ] `src/utils/config_loader.py` — `ConfigLoader` sınıfı
  - `config/settings.json` yükleme ve doğrulama
  - `config/gcs_defaults.json` yükleme
  - `config/profiles/*.json` yükleme ve şema doğrulama
  - Hatalı/eksik config için anlamlı hata mesajları
- [ ] `config/settings.json` — şemayı ve varsayılan değerleri doldur
  - `deployment_mode`: `"simulation"` | `"real"` (Architecture.md §1.3 ile uyumlu)
  - `network.telemetry_endpoint` (UDP host/port — real mod)
  - `network.companion_endpoint` (TCP host/port — real mod)
  - `network.command_endpoint` (TCP host/port — real mod)
  - `network.stream` (host/port/format — real mod)
  - `ros2.bridge.host` + `ros2.bridge.port` (simulation mod)
  - `ros2.topics.*` (telemetry/vehicle_state/mission_state/perception_output/safety_events/camera/commands)
  - `simulation.coordinate_transform` (transform_id, axis_map, translation_m, rotation_deg, scale)
  - `stream.mode` (OFF/OUTPUTS_ONLY/COMPRESSED_LIVE/RAW_DEBUG)
  - `active_profile`: `"default"`
  - `logging.directory`
- [ ] `config/profiles/default.json` — varsayılan profile şeması
  - Event code → severity → action eşlemesi
- [ ] `config/profiles/safe.json` — muhafazakar eşikler
- [ ] `config/profiles/aggressive.json` — izin verici eşikler
- [ ] `config/profiles/README.md` — profile yazarlık kılavuzu

### Loglama Altyapısı

- [ ] `src/utils/logger.py` — merkezi logger kurulumu
  - Yapılandırılmış log formatı (ISO8601 timestamp, seviye, modül, mesaj)
  - Dosya + konsol handler'ları
  - Dönen dosya handler (günlük, maks 7 dosya)
  - `get_logger(name)` fabrika fonksiyonu

### Yardımcı Araçlar

- [ ] `src/utils/json_helpers.py`
  - `load_json(path)` — hata yönetimiyle JSON yükleme
  - `validate_schema(data, schema)` — jsonschema ile doğrulama
- [ ] `src/utils/__init__.py` — public API dışa aktarımları

### Eksik Modeller

- [ ] `src/models/safety_event.py` — `SafetyEvent` dataclass
  - `severity: Severity`, `code: str`, `message: str`, `timestamp: str`
  - `recommended_action: Optional[str]`
  - PROTOCOL.md §6.4 şemasıyla uyumlu
- [ ] `src/models/command_result.py` — `CommandResult` dataclass
  - `status: CommandStatus` (ACK/REJECT/TIMEOUT)
  - `error_code: Optional[str]`, `message: Optional[str]`
  - PROTOCOL.md §7 yaşam döngüsüyle uyumlu
- [ ] `src/models/enums.py` — `CommandStatus` enum ekle (ACK, REJECT, ACK_TIMEOUT, EXEC_TIMEOUT)
- [ ] `src/models/__init__.py` — tüm model sınıflarını dışa aktar

### Test Altyapısı

- [ ] `pyproject.toml` — proje metadata + tüm bağımlılıklar
  - pytest, pytest-cov, jsonschema, PyQt6, pymavlink, roslibpy, opencv-python
- [ ] `tests/` yeniden yapılandırma — C++ testlerini arşivle
  - `tests/legacy_cpp/` altına taşı
  - `tests/unit/` ve `tests/integration/` oluştur
- [ ] `tests/unit/test_config_loader.py`
- [ ] `tests/unit/test_logger.py`
- [ ] `tests/unit/test_models.py` — tüm model sınıflarını test et (SafetyEvent ve CommandResult dahil)

---

## Kabul Kriterleri

- `pytest tests/unit/` tüm testler geçiyor
- Hatalı `settings.json` ile uygulama anlamlı hata mesajı veriyor
- Logger hem konsola hem dosyaya yazıyor
- Profile yükleme: `default.json` yükleniyor, bilinmeyen alanlar uyarı veriyor

---

## Bağımlılıklar

- Önceki sprint yok (ilk sprint)

---

## Notlar

- `config/settings.json` şeması `docs/design/Architecture.md` §1.3 ve `docs/design/PROTOCOL.md` §2.2 ile birebir uyumlu olmalı
- `deployment_mode` değerleri kesinlikle `"simulation"` ve `"real"` olmalı (`"sim"`/`"drone"` değil)
- Profile şeması `docs/spec/exception-handling.md` ile uyumlu olmalı
- `SafetyEvent` modeli `docs/design/PROTOCOL.md` §6.4 şemasıyla tutarlı olmalı
