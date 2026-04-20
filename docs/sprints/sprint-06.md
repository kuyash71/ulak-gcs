# Sprint 6 — Entegrasyon ve Dağıtım

**Faz:** IV  
**Durum:** 🔲 Planlandı  
**Önkoşul:** Sprint 5 tamamlanmış

---

## Hedef

Tam uçtan uca test, SIM ve DRONE modlarının doğrulanması ve
dağıtılabilir tek dosya binary oluşturma.

---

## Görevler

### Uçtan Uca Test (SIM Modu)

- [ ] SITL + Gazebo ortamıyla entegrasyon testi hazırla
- [ ] `tests/integration/test_sim_mode.py`
  - GCS'in SITL'e bağlandığını doğrula
  - `TelemetryFrame` verisi alındığını doğrula
  - Komut gönderim + ACK döngüsünü doğrula
  - Panik butonunun RTL SET_MODE gönderdiğini doğrula
- [ ] ROS2 rosbridge bağlantısını doğrula (isteğe bağlı)

### Uçtan Uca Test (DRONE Modu)

- [ ] HIL (Hardware-in-the-Loop) veya gerçek Pixhawk ile bağlantı testi
- [ ] Raspberry Pi 4B TCP JSON endpoint bağlantısını doğrula
- [ ] `tests/integration/test_drone_mode.py` (donanım varsa)

### Performans ve Stabilite

- [ ] Telemetri güncellemelerinde UI donma yok (50Hz güncelleme profil testi)
- [ ] 1 saatlik SITL oturumunda bellek sızıntısı yok
- [ ] Bağlantı kesilmesi → yeniden bağlanma döngüsünü test et

### CI/CD Pipeline

- [ ] `.github/workflows/ci.yml` — Python test pipeline
  - `pytest tests/unit/` tüm platformlarda
  - `ruff` veya `flake8` linting
  - `mypy` tip kontrolü (isteğe bağlı)
- [ ] GitHub Actions badge'i README'ye ekle

### Dağıtım

- [ ] `scripts/build.py` — PyInstaller build betiği
  - SIM modu binary: `ulak-gcs-sim`
  - DRONE modu binary: `ulak-gcs-drone`
  - Tek klasör çıktısı (config, assets dahil)
- [ ] `scripts/install.sh` — Linux kurulum betiği (isteğe bağlı)

### Dokümantasyon Güncellemeleri

- [ ] `README.md` — kurulum ve çalıştırma talimatlarını güncelle
- [ ] `docs/design/Architecture.md` — implement edilen ve planlanan arasındaki farkları kapat
- [ ] `config/profiles/README.md` — profile yazarlık kılavuzunu doldur
- [ ] `docs/adr/001-customizable-gcs-vision.md` — tamamla
- [ ] `docs/adr/004-polisher-ecosystem.md` — tamamla

---

## Kabul Kriterleri

- `pytest tests/unit/` %100 geçiş, `tests/integration/` SITL ile geçiş
- `ulak-gcs-sim` binary SITL'e bağlanıyor, telemetri alıyor
- Panik butonu SITL üzerinde araçı RTL'e geçiriyor
- CI pipeline başarılı geçiş veriyor
- Bir saatlik SIM oturumunda bellek sızıntısı yok

---

## Bağımlılıklar

- Sprint 5: Tüm UI ve comms bileşenleri
- Dış: SITL kurulumu, PyInstaller

---

## Sonraki Adımlar (Faz 8)

- Replay modu (session log'larından akış tekrar oynatma)
- Operatör kontrol listeleri (uçuş öncesi / uçuş / iniş)
- Lua plugin sistemi (gelişmiş genişletilebilirlik)
- Persistent misyon zaman çizelgesi (filtrelenebilir geçmiş)
