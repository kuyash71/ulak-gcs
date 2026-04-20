# Sprint 5 — UI Tamamlama ve Polisher

**Faz:** III  
**Durum:** 🔲 Planlandı  
**Önkoşul:** Sprint 4 tamamlanmış

---

## Hedef

Video stream paneli, Polisher geliştirici aracı ve SIM modu topic eşleme
panelini ekleyerek UI'ı eksiksiz hale getirmek. Sprint sonu itibarıyla
arayüz MVP seviyesine ulaşmış olacak.

---

## Görevler

### Video Stream Paneli

- [ ] `src/ui/panels/stream_panel.py` — `StreamPanel(QWidget)`
  - Kamera akışını görüntüle (MJPEG veya raw frame)
  - Stream modu seçici: OFF / OUTPUTS_ONLY / COMPRESSED_LIVE / RAW_DEBUG
  - Akış kesildiğinde "Sinyal Yok" placeholder
  - FPS ve gecikme göstergesi
- [ ] `src/comms/video_stream_component.py` — `StreamManager`
  - **Real mod**: H.264/H.265 TCP akışını OpenCV/GStreamer ile decode et (`network.stream`)
  - **Simulation mod**: ROS2 `sensor_msgs/Image` base64 frame'lerini rosbridge üzerinden decode et
  - `StreamMode` değişimini destekle; mode değişimi CC'ye komut gönderir
  - **Frame drop stratejisi**: yük altında en eski frame'i düşür; UI thread hiçbir zaman bloklanmaz
  - Frame queue boyutu yapılandırılabilir (varsayılan: 3 frame)
  - FPS ve gecikme metriklerini AppController'a yayınla

### Polisher Paneli

- [ ] `src/ui/panels/polisher_panel.py` — `PolisherPanel(QWidget)`
  - Mevcut script dizinini listele
  - Seçili scriptteki `@polisher_param` bloklarını parse et
  - Her parametre için dinamik kontrol üret (slider, spinbox, checkbox)
  - Parametre değişimlerini runtime'da scripte uygula
  - `docs/spec/polisher.md` sözleşmesine uyum
- [ ] `src/utils/polisher_parser.py`
  - Python kaynak dosyasından `@polisher_start`/`@polisher_end` bloklarını oku
  - `@polisher_param` direktiflerini parse et (tip, min, max, varsayılan)

### ROS2 Topic Eşleme Paneli (Yalnızca SIM Modu)

- [ ] `src/ui/panels/ros2_mapping_panel.py` — `Ros2MappingPanel(QWidget)`
  - Yapılandırılmış topic eşlemelerini listele (PROTOCOL.md §11.2 varsayılan tablosuyla)
  - Operatörün topic isimlerini düzenlemesine izin ver
  - Kaydedilmemiş değişiklik varken **"⚠ Restart required to apply changes"** banner göster (Architecture.md §1.3.1)
  - Değişiklikleri `settings.json`'a kaydet; uygulama yeniden başlatıldığında etkin olur
  - Yalnızca `deployment_mode == "simulation"` olduğunda görünür

### Profile Değiştirme UI

- [ ] `src/ui/widgets/profile_selector.py`
  - Mevcut profile'ları listeleyen açılır menü
  - `ProfileManager.load_profile()` çağırır
  - Durum çubuğunda aktif profile'ı günceller

### Misyon Zaman Çizelgesi (Opsiyonel)

- [ ] `src/ui/panels/timeline_panel.py` — `TimelinePanel(QWidget)`
  - Kaydırılabilir misyon olayları listesi (zaman damgası + açıklama)
  - Seviyeye göre filtre (WARN / ERROR / CRITICAL / Tümü)
  - Session log dosyasından doldurulur

### Testler

- [ ] `tests/unit/test_polisher_parser.py` — `@polisher_param` parsing edge case'leri
- [ ] `tests/unit/test_stream_panel.py` — mod değişimi mantığını test et

---

## Kabul Kriterleri

- Video stream paneli MJPEG akışını render ediyor
- Polisher script listesini okuyup `@polisher_param` kontrollerini dinamik oluşturuyor
- ROS2 eşleme paneli yalnızca SIM modunda görünüyor
- Profile değişimi crash olmadan çalışıyor
- Tüm UI bileşenleri `pytest` ile test ediliyor (headless/mock)

---

## Bağımlılıklar

- Sprint 4: Ana pencere, dock sistemi, tema
- Sprint 3: VideoStreamComponent backend
