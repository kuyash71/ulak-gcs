# Sprint 3 — İletişim Katmanı

**Faz:** II  
**Durum:** 🔲 Planlandı  
**Önkoşul:** Sprint 2 tamamlanmış

---

## Hedef

Drone ve simülatörden gerçek veri alacak, komut gönderecek tüm iletişim
bileşenlerini uygulamak. Sprint sonu itibarıyla gerçek MAVLink telemetrisi
alınabilmeli ve komut gönderilebilmelidir.

---

## Görevler

### Worker Thread Mimarisi

- [ ] Her comms backend kendi `QThread` worker'ında çalışır
  - UI thread'i hiçbir IO'ya dokunmaz (Architecture.md §2.2 "UI ≠ IO" prensibi)
  - Backend → AppController iletişimi PyQt6 sinyalleri ile (thread-safe)
  - Worker başlatma/durdurma `AppController` sorumluluğunda

### Coordinate Transformer

- [ ] `src/utils/coord_transformer.py` — `CoordinateTransformer`
  - PROTOCOL.md §6.1.4 transform şemasını uygula
  - `transform_id`, `axis_map`, `translation_m`, `rotation_deg`, `scale` destekle
  - `transform(position: Vector3, velocity: Vector3) -> (Vector3, Vector3)` metodu
  - Gazebo→ArduPilot LOCAL_NED dönüşümü için varsayılan config
  - Runtime transform değişimini destekle (audit log ile)

### Backend Soyutlaması

- [ ] `src/comms/base.py` — soyut temel sınıflar
  - `BaseTelemetryBackend(ABC)` — `connect()`, `disconnect()`, `on_frame(callback)`
  - `BaseMissionBackend(ABC)` — `connect()`, `on_mission_state(callback)`, `on_perception(callback)`, `on_safety_event(callback)`
  - `BaseCommandBackend(ABC)` — `send_command(cmd) -> CommandResult`
  - `BaseStreamBackend(ABC)` — `connect()`, `set_mode(StreamMode)`, `on_frame(callback)`
  - Tüm backend'lerde reconnect + backoff: `_reconnect_with_backoff()` (500ms→1s→2s→üstel, maks 30s)

### TelemetryComponent

- [ ] `src/comms/telemetry_mavlink.py` — MAVLink/UDP backend
  - pymavlink ile UDP dinleme
  - HEARTBEAT, ATTITUDE, LOCAL_POSITION_NED, BATTERY_STATUS mesajlarını parse et
  - `TelemetryFrame` dataclass'ına normalize et (XYZ kanonik sözleşme)
  - Bağlantı kesildiğinde `CRITICAL` exception yayınla
- [ ] `src/comms/telemetry_ros2.py` — ROS2/rosbridge backend
  - roslibpy ile WebSocket bağlantısı (`ros2.bridge.host:port`)
  - `geometry_msgs/PoseStamped` → `TelemetryFrame` normalize et
  - `mavros_msgs/State` → `vehicle_mode` ve arm durumunu ekle
  - `CoordinateTransformer` ile Gazebo→ArduPilot dönüşümü uygula (simulation modunda)
  - `docs/design/PROTOCOL.md` §11.2 varsayılan topic eşlemesine uyum

### MissionComponent

- [ ] `src/comms/mission_component.py`
  - TCP JSON soketi (Raspberry Pi 4B'ye bağlanır)
  - `MissionState` güncellemelerini parse et
  - `PerceptionTarget` güncellemelerini parse et
  - Safety event mesajlarını parse et → `ExceptionClassifier`'a ilet
  - `docs/design/PROTOCOL.md` envelope şemasına uyum

### CommandGateway

- [ ] `src/comms/command_gateway.py`
  - Komutları doğrula (bilinmeyen komutları `UNSUPPORTED_COMMAND` ile reddet)
  - JSON envelope oluştur (schema_version, category, timestamp, source, correlation_id)
  - TCP JSON üzerinden CC'ye komut gönder (real mod)
  - ROS2 `ros2.topics.commands` topic'ine komut gönder (simulation mod)
  - ACK/REJECT/TIMEOUT döngüsünü yönet: `T_ack=2s`, `T_exec=10s`
  - Retry policy: `TARGET_UNREACHABLE`/`TARGET_BUSY`/`ACK_TIMEOUT` için üstel backoff (500ms→1s→2s, maks 3 deneme)
  - `correlation_id` ile 60s idempotency window — aynı id ile gelen yanıtı döndür
  - Panik komutu `PanicManager`'a yönlendir (CommandGateway'den geçmez)
  - Her gönderilen komutu audit log'a yaz
  - `docs/design/PROTOCOL.md` §7, §9 komut yaşam döngüsü ve retry kurallarına uyum
- [ ] `SET_SIMULATOR_COORD_TRANSFORM` komutu desteği
  - `CoordinateTransformer`'ı runtime'da güncelle
  - PROTOCOL.md §10.6 payload şemasıyla uyumlu

### Backend Fabrikası

- [ ] `src/comms/factory.py`
  - `create_telemetry_backend(config) -> BaseTelemetryBackend`
  - `create_mission_backend(config) -> BaseMissionBackend`
  - `create_command_backend(config) -> BaseCommandBackend`
  - `create_stream_backend(config) -> BaseStreamBackend`
  - Config'deki `deployment_mode` alanına göre (`"simulation"` | `"real"`) doğru backend'i seç

### AppController Entegrasyonu

- [ ] `AppController`'ı güncelle — comms backend'lerini başlat ve callback'leri bağla

### Testler

- [ ] `tests/unit/test_telemetry_mavlink.py` — kayıtlı MAVLink paketleriyle mock test
- [ ] `tests/unit/test_telemetry_ros2.py` — mock rosbridge mesajlarıyla test
- [ ] `tests/unit/test_coord_transformer.py` — Gazebo→ArduPilot eksen dönüşüm doğruluğu
- [ ] `tests/unit/test_mission_component.py` — örnek JSON payload'larıyla (SafetyEvent dahil)
- [ ] `tests/unit/test_command_gateway.py` — ACK/REJECT/TIMEOUT, retry, idempotency senaryoları
- [ ] `tests/integration/` — SITL ile bağlantı testi (isteğe bağlı, CI dışında)

---

## Kabul Kriterleri

- `TelemetryComponent` MAVLink HEARTBEAT alıyor, `TelemetryFrame` oluşturuyor; tüm IO QThread'de
- ROS2 backend aynı `BaseTelemetryBackend` arayüzünü uyguluyor; `CoordinateTransformer` uygulanıyor
- `MissionComponent` TCP/ROS2 üzerinden `MissionState`, `PerceptionTarget`, `SafetyEvent` parse ediyor
- `CommandGateway` geçersiz komutları reddediyor, ACK/REJECT/TIMEOUT döngüsünü yönetiyor
- Retry policy çalışıyor: `ACK_TIMEOUT` sonrası maks 3 deneme, exponential backoff
- Aynı `correlation_id` ile gelen ikinci request, 60s içinde aynı yanıtı döndürüyor
- Bağlantı kopması `AppController`'a `CRITICAL` `SafetyEvent` olarak yansıyor
- Backend değişimi yalnızca config (`deployment_mode`) değişimiyle tetikleniyor

---

## Bağımlılıklar

- Sprint 2: AppController, ExceptionClassifier, PanicManager
- Dış: pymavlink, roslibpy
