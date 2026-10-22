# NOAR UE5 Digital Apartment Standard v1.0

Oct 22, 2026 · @Artur

**Production engine: Unreal Engine 5.8.x**, Mac: Xcode 26.1.1 (минимум 26.0; Xcode 26.4 несовместим с UE). Смена minor/major версии движка посреди проекта — только через отдельный тестовый branch. Версия документа 1.1: внесены правки по рецензии (Substrate, Mac HWRT/MegaLights, Lumen Lite, Planar, Datasmith, BLUM-кинематика, Asset Manager, MetaHuman на Gate 2).

## 0. Анализ video reference

Референс — это UE5 **Third Person Template** (манекен Manny, Lumen, PIE-окно редактора) в люксовом двухуровневом интерьере: камера, персонаж и скорость взяты из шаблона почти без изменений, а «дорогой» вид делают свет, материалы и плотность декора. Запись: 27 с, экран монитора снят на телефон (546×512, 30 fps), поэтому цифры ниже — оценка по кадрам, а не измерение.

### Camera

| Параметр | Наблюдение в референсе | Вывод для NOAR |
| --- | --- | --- |
| Режим | Third person, камера сзади-сверху | Third person по умолчанию, first person по клавише |
| Target Arm Length | ≈ 350–400 см (шаблон: 400) | 320 см — ближе, интерьер читается лучше |
| Высота / pitch | Камера выше головы, pitch ≈ −20…−35°, в поворотах до −45° (вид почти сверху) | Pitch по умолчанию −15°, ограничить −60…+20° |
| FOV | ≈ 90° (шаблон), заметно широкоугольный, стены «разъезжаются» по краям | 75° (эквивалент ≈ 24 мм на full frame) — архитектурный, без искажений |
| Camera lag | Есть: камера плавно «догоняет» персонажа после поворота, Rotation lag отсутствует (камера жёстко следует мыши) | Camera Lag ON (speed 8), Rotation Lag ON (speed 12) |
| Скорость вращения | Высокая, мышь 1:1, резкие развороты на 90–180° за 0.3 с | Снизить чувствительность на 40 %, добавить сглаживание |
| Motion blur | Включён, в быстрых поворотах кадры сильно смазаны | Motion blur 0.25 в walkthrough, 0 в photo mode |
| Поведение у стен | Spring Arm collision: камера «впрыгивает» к персонажу у стен и в коридорах | Probe 12 см + плавный zoom-in, без рывка |

### Character

| Параметр | Референс | NOAR |
| --- | --- | --- |
| Скорость | ≈ 500 см/с (бег шаблона), за 1 с персонаж проходит 2 плитки 60 см ×4 | 180 см/с ходьба, 300 см/с «быстрый шаг» по Shift |
| Acceleration / braking | Шаблон: 2048 / 2048 — старт и стоп мгновенные | 600 / 800 — плавный старт, короткий тормозной путь |
| Rotation rate | 500°/с, разворот на месте | 360°/с |
| Controller yaw | OFF; **Orient Rotation to Movement ON** — персонаж поворачивается по направлению движения, камера свободна | Так же: это и есть «архитектурное» ощущение, а не шутер |
| Strafing | Нет (персонаж разворачивается) | Нет в third person; есть в first person |
| Locomotion | Blend Space 1D Idle→Walk→Run шаблона, без turn-in-place | Blend Space 1D Idle/Walk/FastWalk + turn-in-place |
| Ощущение | Videogame: слишком быстро, развороты резкие | Цель — спокойная прогулка, как человек в своей квартире |

### Visual style

- **GI**: Lumen (Software RT), тёплый отражённый свет от дерева на потолке и белых стенах — главный источник «дорогого» ощущения.
- **Reflections**: Lumen reflections, размытые отражения на мраморе острова и стекле витрины; зеркал в кадре нет.
- **Shadows**: Virtual Shadow Maps, мягкие контактные тени под мебелью, без «лесенки».
- **Exposure**: Auto Exposure (histogram) с заметной адаптацией при повороте к окнам; окна пересвечены, но не выбелены полностью — EV100 примерно 11–12 в комнате.
- **White balance / контраст**: тёплый (≈ 5000–5500 K), контраст умеренный, тени не задавлены, блики мягкие.
- **Bloom**: низкий (0.3–0.4), светится только LED-подсветка и оконный проём.
- **Saturation**: чуть выше нейтрали — оранжевые кресла и дерево «звучат».
- **Volumetrics / атмосфера**: нет; глубина достигается Lumen AO и контрастом «тёплое дерево — светлый камень».
- **Детализация**: плотный декор (книги, растения, вино, посуда), ткани, шторы; ни одной пустой стены.

### Materials в референсе

| Материал | Что видно | Почему работает |
| --- | --- | --- |
| Wood / veneer | Тёмный орех, панели с криволинейным рельефом, стеновые ламели | Правильный масштаб волокна (≈ 30 см повтор), roughness ≈ 0.35–0.45, свет ложится на фаски |
| Stone / marble | Белый мрамор с серой жилой на острове и столе, waterfall edge | Один слэб без повторов, Lumen-отражения с roughness 0.15–0.2 |
| Glass | Витрина вина — bronze tinted glass с LED-полками | Тонировка + внутренняя подсветка = объём |
| Mirror | Нет в кадре | — |
| Metal | Хромовые кольца люстры, чёрные профили | Roughness 0.1–0.2, сильный Fresnel |
| Painted surfaces | Белые стены, потолок с подсветкой карниза | Roughness 0.6–0.7, лёгкая micro-normal, не «пластик» |
| Fabric | Букле-диван, бархатные оранжевые стулья, тюль | Fuzz / sheen, нормаль ткани, roughness 0.8+ |
| Floor | Светлый камень / паркет крупного формата | Низкий контраст швов, отражения Lumen |
| Wall panels | Деревянные панели с 3D-волной | Геометрия, а не normal map — ловит свет |

**Почему интерьер выглядит дорого**: (1) тёплый bounce от дерева заливает белые плоскости; (2) материалы в физически правильном масштабе; (3) ничего не острое — фаски на панелях и столешницах; (4) ни одной пустой поверхности — декор, текстиль, растения; (5) один тёплый white balance на всю сцену. Это и есть quality target NOAR: не дизайн референса, а эти пять свойств.

**Где NOAR должен быть лучше референса**: FOV 90° и бег 500 см/с — это шаблон, а не выбор; у нас камера 75°, ходьба 180 см/с, rotation lag, exposure без скачков и photo mode без motion blur.

## 1. Project Settings

Проект создаётся из шаблона **Games → Third Person, Blueprint, Maximum quality, Raytracing OFF, Starter Content OFF**, UE 5.8.x (зафиксировано; настройки ниже обоснованы состоянием 5.8). Всё, что зависит от платформы, задаётся в `DefaultEngine.ini` один раз и не меняется в процессе.

| Настройка | Windows | macOS (Apple Silicon) | Почему |
| --- | --- | --- | --- |
| Default RHI | DirectX 12, SM6 | Metal, SM6 | SM6 нужен для Nanite, Lumen и VSM |
| Dynamic Global Illumination | Lumen | Lumen | Единственный real-time GI нужного качества; никакого Lightmass — конфигуратор меняет материалы в runtime |
| Reflection Method | Lumen | Lumen | Единая система с GI |
| Software Ray Tracing Mode | Detail Tracing | Detail Tracing | Мелкая мебель и фаски в GI |
| Hardware Ray Tracing | ON (как опция для RTX) | OFF | На Mac HWRT и MegaLights — experimental (Apple Silicon M2+), не для production; на RTX даёт точные отражения стекла и зеркал |
| Ray Traced Shadows | OFF | OFF | VSM даёт нужную мягкость дешевле |
| Shadow Map Method | Virtual Shadow Maps | Virtual Shadow Maps | Контактные тени под мебелью без лесенки |
| Nanite | ON | ON | Архитектура, фаски, декор без LOD |
| Rendering path | Deferred | Deferred | Lumen требует Deferred; Forward — только для будущего VR-билда |
| Anti-Aliasing | TSR | TSR | Единственный AA, который держит тонкие фаски и тени VSM; DLSS — плагин только для RTX |
| Screen Percentage | 100 (TSR upscale от 67 на LOW) | 67–75 | Mac тянет 4K только через upscale |
| Texture Streaming | ON, pool стартово 4000 MB (RTX) — значение profile-derived, проверяется stat streaming | ON, pool стартово 2500 MB — profile-derived | Без пула 8K-текстуры выбьют VRAM |
| Virtual Textures | ON, только для hero-текстур ≥ 4K (SVT); остальное — обычный streaming | ON | Слэбы камня 8K и дерево 4K |
| Auto Exposure | ON, Extend default luminance range ON | то же | Физические единицы EV100 |
| Support Hardware Variable Rate Shading | OFF | OFF | Артефакты на фасках |
| Motion Blur | ON (скейл в PP) | ON | Контролируется в PP\_NOAR\_MASTER |
| Generate Mesh Distance Fields | ON | ON | Требуется Software Lumen |
| Substrate | ON — Blendable GBuffer (production default) | ON — Blendable GBuffer | В 5.8 Substrate включён для новых проектов по умолчанию; Blendable GBuffer — parity по performance с legacy. Материалы держим простыми. Adaptive GBuffer — опциональный quality-профиль для hero-материалов только после профилирования (цена зависит от сложности видимых материалов) |
| Enhanced Input | ON | ON | Единственная актуальная система ввода |
| Start in VR | OFF | OFF | VR — отдельный target в будущем |

### Ключевые CVars (DefaultEngine.ini, секция \[/Script/Engine.RendererSettings\] и DefaultScalability.ini)

```ini
r.DynamicGlobalIlluminationMethod=1
r.ReflectionMethod=1
r.Lumen.TraceMeshSDFs=1
r.Lumen.Reflections.Allow=1
r.Shadow.Virtual.Enable=1
r.Nanite=1
r.AntiAliasingMethod=4
r.DefaultFeature.AutoExposure.ExtendDefaultLuminanceRange=1
r.VirtualTextures=1
r.VT.TileSize=128
r.Streaming.PoolSize=4000
r.GenerateMeshDistanceFields=1
r.DistanceFields.DefaultVoxelDensity=0.2
r.SkinCache.CompileShaders=1
```

На Mac добавляется `[Mac DeviceProfile]` с `r.Streaming.PoolSize=2500`, `r.ScreenPercentage=75`, `r.Lumen.Reflections.DownsampleFactor=2`. Один ini — два device profile; никаких ручных переключений перед билдом.

## 2. Folder structure и naming

Правило одно: **общее — в `Content/NOAR/`, квартирное — в `Content/Apartments/<ID>/`**. Фреймворк (блупринты, мастер-материалы, персонаж, UI) никогда не ссылается на конкретную квартиру, поэтому вторая и сотая квартира добавляются как папка, а не как проект.

```text
Content/
  NOAR/                          # фреймворк, переиспользуется во всех квартирах
    Core/                        # GameMode, PlayerController, GameInstance, SaveGame
    Characters/NOAR_Walker/      # SK_, ABP_, BS_, анимации
    Interaction/                 # BPI_Interactable, BP_InteractionComponent, BP_InteractiveDoor и т.д.
      Doors/  Drawers/  Mechanisms/  Appliances/  Lights/
    Configurator/                # BP_MaterialConfigurator, DA_MaterialOption, DT_FinishOptions
    Materials/
      Master/                    # M_Master_*
      Functions/                 # MF_*
      Library/<Category>/        # MI_* общие финиши (дуб, орех, камень) + T_*
    Lighting/                    # IES, BP_InteractiveLight, BP_LightSwitch, BP_DayNightManager, DA_LightPreset
    PostProcess/                 # PP_NOAR_MASTER, LUT
    UI/                          # WBP_*, шрифты, иконки
    Audio/                       # ambience, soft-close, MetaSounds
    FX/                          # NS_* (дым свечи, пыль в луче — если понадобится)
    Input/                       # IMC_*, IA_*
    Debug/                       # WBP_DebugMenu — исключается из shipping
  Apartments/
    DEMPSTER/                    # одна квартира = одна папка
      Maps/                      # L_DEMPSTER_Main + sublevels
      Architecture/              # Datasmith-импорт: стены, полы, потолки, окна
      Millwork/Static/           # корпуса, панели, полки
      Millwork/Interactive/      # фасады, ящики, подъёмные дверцы (SM_ с правильными pivot)
      Furniture/  Decor/  Appliances/  Fixtures/   # светильники, сантехника
      Materials/                 # MI_* специфичные для этой квартиры (SS02 Cipollino, LAM02 Cashmere Gold)
      Data/                      # DA_Apartment_DEMPSTER, DA_Room_*, DT_FinishOptions_DEMPSTER
      Cinematics/                # LS_* секвенсы, CineCamera-пресеты
  Developers/<name>/             # личные песочницы, не попадают в билд
```

### Naming convention (ассеты)

| Префикс | Тип | Пример |
| --- | --- | --- |
| SM\_ | Static Mesh | SM\_DEM\_KIT\_B01\_DOOR\_L |
| SK\_ / ABP\_ / BS\_ / AM\_ | Skeletal mesh, Anim BP, Blend Space, Montage | SK\_NOAR\_Walker |
| M\_ / MI\_ / MF\_ | Master material, instance, function | M\_Master\_Wood, MI\_Oak\_RiftSawn\_M01 |
| T\_ … \_BC / \_N / \_R / \_AO / \_H / \_ORM | Текстуры по каналам | T\_Oak\_Rift\_4K\_N |
| BP\_ / BPI\_ / BPC\_ | Blueprint, interface, component | BPC\_Interaction |
| DA\_ / DT\_ / E\_ / S\_ | Data asset, table, enum, struct | DA\_MaterialOption\_Oak |
| WBP\_ | Widget | WBP\_InteractionPrompt |
| L\_ / LS\_ | Level, Level Sequence | L\_DEMPSTER\_Main |
| PP\_ / LUT\_ | Post process, lookup | PP\_NOAR\_MASTER |
| IES\_ / RT\_ / NS\_ / SFX\_ / MS\_ | IES, render target, Niagara, звук, MetaSound | IES\_Downlight\_24deg |
| IMC\_ / IA\_ | Input mapping context, action | IA\_Interact |
| SG\_ | SaveGame | SG\_NOAR\_ClientConfiguration |

### Naming интерактивной мебели (задаётся в CAD до экспорта)

Формат: `SM_<APT>_<ROOM>_<UNIT>_<PART>[_<INDEX>][_<SIDE>]`. Имя — это ключ, по которому BP\_ApartmentManager при реимпорте находит свои акторы, поэтому оно не меняется никогда.

| Сегмент | Значения |
| --- | --- |
| APT | DEM (Dempster), далее код каждой квартиры, 3 буквы |
| ROOM | FOY, KIT, DIN, LIV, PBR (primary bedroom), WIC, PEN (primary ensuite), GBR, GEN, DEN, PWD, LAU, BAL |
| UNIT | B01… base, W01… wall, T01… tall, I01 island, V01 vanity, P01 panel, S01 shelving, A01 appliance |
| PART | CARC (корпус), DOOR, DRW, FLAP (AVENTOS), SLIDE, POCKET, INT (наполнение), LED, SHELF, TRAY, TOP (столешница) |
| INDEX | 01… сверху вниз для ящиков, слева направо для дверей |
| SIDE | L / R (навес петли), T (top), B (bottom) |

Пример кухонной базы Dempster:

```text
SM_DEM_KIT_B03_CARC
SM_DEM_KIT_B03_DOOR_01_L
SM_DEM_KIT_B03_DRW_01   # LEGRABOX верхний
SM_DEM_KIT_B03_DRW_02
SM_DEM_KIT_B03_TRAY_01  # внутренний лоток
SM_DEM_KIT_B03_INT
SM_DEM_KIT_B03_LED
```

В уровне акторы называются так же без `SM_` (`DEM_KIT_B03_DRW_01`), а группы света — `LG_<APT>_<ROOM>_<TYPE>` (LG\_DEM\_KIT\_UNDERCAB). При 100+ дверей и 50+ LED-групп это единственный способ найти объект поиском в Outliner.

## 3. Import pipeline

Архитектура идёт через **Datasmith** (сохраняет иерархию, материалы и поддерживает реимпорт), интерактивная мебель — через **FBX поштучно** (полный контроль pivot), декор — через FBX/glTF из библиотек. Единицы везде сантиметры, ось Z вверх, начало координат — угол пола у входной двери D.01 (один origin для всех экспортов квартиры).

| Слой | Источник | Формат | Как импортируется | Nanite |
| --- | --- | --- | --- | --- |
| ARCHITECTURE (стены, пол, потолки, проёмы, балкон) | Revit / SketchUp / 3ds Max | Datasmith (.udatasmith) | Один Datasmith Scene Actor в Sublevel\_Architecture; реимпорт обновляет меши на месте | YES |
| STATIC MILLWORK (корпуса, стеновые панели M01, полки, столешницы) | Cabinet Vision / Mozaik / 3ds Max | FBX, один файл на комнату | Import as separate meshes, Combine OFF, pivot в мировом origin | YES |
| INTERACTIVE MILLWORK (фасады, ящики, подъёмные, раздвижные) | то же | FBX, один меш = одна подвижная деталь | Pivot на оси механизма (раздел 4), Transform Vertex to Absolute OFF | YES (кроме стеклянных) |
| DECOR / FURNITURE | Библиотеки производителей, Megascans, свои модели | FBX / glTF | В Content/Apartments/\<ID>/Furniture, материалы переназначаются на MI\_ библиотеки | YES (кроме ткани с WPO и стекла) |
| LIGHTS | Электрический план D1.02 | Ручная расстановка в UE | Только меши светильников из FBX; сами Light-компоненты — внутри BP\_InteractiveLight | — |
| GLASS | вместе с мебелью | FBX, отдельный меш от рамы | Свой material slot, толщина реальная (6–10 мм), нормали наружу | NO |
| MIRRORS | вместе с мебелью | FBX, отдельный плоский меш | Отдельный актор (нужен Planar Reflection для hero) | NO |

### Что нельзя делать в CAD перед экспортом

- Склеивать корпус с фасадами в один меш — потом не разделить.
- Оставлять negative или non-uniform scale и «грязные» transforms на подвижных деталях: локальные оси стандартизированы (X наружу, Z вверх), pivot на оси механизма сохраняется, каждый экспорт проходит автоматическую проверку pivot/scale (Editor Utility). Как именно это достигается (Reset XForm или нет) — зависит от DCC.
- Дублировать геометрию стен в миллворк-файле.
- Оставлять материалы с именами типа `Material #23` — имя материала в CAD = код финиша из спецификации (`M01_Oak`, `LAM01_LightGrey`, `SS01_Estatuario`, `GL01_BronzeMirror`); при импорте по этому имени автоматически подставляется MI.

### Обновление модели без поломки интерактива

Интерактивность живёт не в мешах, а в **акторах уровня**, которые ссылаются на меши по имени. Обновление геометрии = замена ассета SM\_ под тем же именем, актор и его логика не трогаются.

1. **Архитектура**: Datasmith Reimport. Стены и полы не несут логики, но реимпорт перезаписывает Static Mesh, Material Instance и Parent Material, созданные Datasmith. Правило: папка Datasmith-импорта (Architecture/Imported) — read-only, никаких ручных production-правок внутри; всё кастомное — дублированные ассеты в NOAR/ или Apartments/\<ID>/Materials, которые назначаются поверх после каждого реимпорта (скрипт по имени материала). Свет, мебель и интерактив лежат в других sublevel и не затрагиваются.
2. **Мебель**: фасад — это актор `BP_InteractiveDoor` с переменной `DoorMesh` (Static Mesh reference). Реимпорт FBX под тем же именем обновляет меш, актор сохраняет OpenAngle, Sound, Locked, связи с LED и конфигуратором.
3. **Новый шкаф**: эдиторский утилитарный блупринт `EUW_MillworkAssembler` читает имена импортированных мешей и по суффиксу (`_DOOR_`, `_DRW_`, `_FLAP_`, `_LED`) создаёт нужные акторы в позиции pivot. Ручная работа — только проверка направления открывания.
4. **Материалы**: привязаны через Material Slot Name, а не индекс; конфигуратор ищет слот `FRONT`, `CARCASS`, `TOP`, поэтому порядок слотов в новом FBX не важен.
5. **Свет и UI**: никогда не ссылаются на геометрию, только на Gameplay Tags (`Zone.Kitchen`, `LightGroup.Undercab`) — см. раздел 10.

Специфика Dempster: существующая белая кухня и плитка пола остаются — их моделируем по фото и плану D1.00 в слой STATIC MILLWORK; новое (панели M01, витрина MT01 с GL02, фертурная стена столовой с бронзовым зеркалом GL01, WIC, Murphy bed в den) — из цеховых чертежей с разбивкой на подвижные части.

## 4. Geometry: pivots, фаски, collision, Nanite, LOD

### Pivots — реальная кинематика, не хаки

Каждый подвижный меш экспортируется с локальной системой координат на оси механизма. Ось X — «наружу из шкафа», Z — вверх. Тогда один Blueprint работает для любого фасада без поправочных смещений.

| Механизм | Положение pivot | Ось движения | Примечание |
| --- | --- | --- | --- |
| Hinged door (Blum CLIP top 110°) | Ось чашки петли: 3–5 мм от внутренней плоскости фасада, на кромке со стороны навеса, по центру высоты | Rotation Z, 0→±105° | Знак угла задаёт L/R |
| Drawer (LEGRABOX / MOVENTO / TANDEM) | Задняя нижняя кромка фасада по центру | Translation X, 0→TravelDistance | Фасад + короб — один меш; внутренний лоток — отдельный child-меш |
| Sliding door (шкаф D.14 guest closet) | Центр верхней кромки на оси трека | Translation Y вдоль трека | Трек — отдельный статичный меш |
| Pocket door (D.10, D.12) | Та же, что sliding | Translation Y в карман | В кармане полотно скрывается геометрией стены, ничего не отключаем |
| Pocket system (мебель, Hawa Concepta) | Два pivot: петля (распах) + каретка (вдвиг) | Rotation Z, затем Translation X | Два вложенных компонента: Carriage → Door |
| Lift-up AVENTOS HK / HK top | Верхняя кромка фасада на оси рычага (≈ верх корпуса минус толщина фасада) | Rotation Y, 0→−90° (вверх) | Одна ось |
| AVENTOS HF (складной) | Верхняя створка: pivot на верхней кромке; нижняя: pivot на стыке створок | Rotation Y обе | Нижняя — child верхней, вращается в обратную сторону ×2 |
| AVENTOS HS (уходит над корпусом) / HL (поднимается параллельно) | Pivot на центре фасада | Траектория — две кривые Timeline (Z-смещение + наклон) | Кривые снимаются с чертежа Blum, не из головы |
| Bi-fold | Каждая створка со своим pivot на своей петле | Rotation Z, вторая створка — child первой | Угол второй = −2× первой |
| Appliances (духовка, посудомойка) | Нижняя кромка двери на оси петли производителя | Rotation Y, 0→90° вниз | Холодильник — как hinged door, 0→115° |
| Murphy bed (den, LAM05) | Ось механизма у пола | Rotation Y, 0→90° | Ножки — отдельный child с своим pivot |

### Фаски (физические, в геометрии; с Nanite они доступны по цене и стоят того — но не бесплатны: кластеры, память, материалы; проверять в Nanite Overdraw)

| Элемент | Фаска | Сегментов | Почему |
| --- | --- | --- | --- |
| Фасады ламинат (LAM01, LAM03…) | 1 мм | 2 | Реальная кромка ABS 1 мм; блик на ребре виден с 1 м |
| Шпонированные панели M01 | 0.5 мм | 1 | Шпон на митре почти острый, но не CG |
| Столешницы Neolith 2 см (SS01), Cipollino 6 мм (SS02) | 2 мм (square edge по спецификации) | 2 | Камень не бывает острым; у тонкого 6 мм — 1 мм |
| Металлические рамы (MT01 bronze, MT02 black) | 0.5 мм | 1 | Тонкий блик по ребру — главный признак металла |
| Стеновые деревянные панели / ребра | 1 мм | 2 | Рифлёные детали ловят свет по всей длине |
| Гипсокартонные углы, ниши | 2–3 мм | 2 | Шпаклёванный угол скруглён |
| Потолочные стрипы MD01 (кухня) | 1 мм | 1 | Крашенный MDF |

### Collision

| Объект | Тип | Почему |
| --- | --- | --- |
| Стены, пол, потолок | Отдельный невидимый меш `SM_DEM_COLLISION_Shell` из боксов (UCX) | Архитектурный Nanite-меш с Complex-as-Simple дорог и цепляет камеру за плинтуса |
| Лестницы (если дуплекс) | Наклонная плоскость (пандус) + боксы перил | Персонаж идёт плавно, без ступенчатой тряски камеры |
| Корпуса мебели, остров | Simple: 1–3 бокса (авто или UCX) | Блокирует персонажа, дешёво |
| Фасады, ящики | Simple box, канал `Interactable` ON, Pawn Ignore | Нужны только для trace курсора; не толкают персонажа при открытии |
| Диваны, кресла, кровать | Simple: convex 4–8 hull | Обтекаемые формы без застревания |
| Стулья, торшеры | Один бокс или капсула | Персонаж не должен вставать на ножки стула |
| Стекло витрин и дверей | Simple box, Camera Block ON | Камера не проходит сквозь стекло двери den |
| Декор на полках, растения, шторы | No collision | Не мешают камере и трейсу |
| Межкомнатные двери D.03–D.15 | Simple box, Pawn Block в закрытом состоянии, Ignore во время анимации | Дверь не сдвигает персонажа, камера не застревает |

Complex-as-Simple — только для одного случая: криволинейный парапет балкона (в Dempster он дугой), если боксами не описывается. Nanite-меши с Complex-as-Simple не работают напрямую — нужен отдельный low-poly collision mesh.

### Nanite YES / NO

| YES | NO | Причина |
| --- | --- | --- |
| Архитектура, корпуса, фасады, ящики (движущиеся — тоже: трансформ актора Nanite поддерживает) | — | Фаски и миллионы треугольников без LOD |
| Камень, столешницы, сантехника, светильники (корпус) | — |  |
| Декор high-poly (посуда, вазы, книги) | — |  |
| Растения — ствол/горшок | Листья с masked-материалом и WPO | Nanite + masked дорог на Mac; листья — обычный меш с LOD |
| — | Стекло, зеркала, тюль (translucent) | Nanite не поддерживает translucent |
| — | Светильники — рассеиватели (emissive translucent) | то же |
| — | Ткань с WPO-анимацией штор | WPO на Nanite дорог; шторы статичные — можно Nanite |
| — | Skeletal mesh персонажа | Nanite skeletal — experimental |

### LOD и HLOD

Для non-Nanite ассетов (листья, стекло, рассеиватели, мелкие аксессуары): LOD0 100 %, LOD1 50 % на screen size 0.35, LOD2 20 % на 0.12, автогенерация в импорте. В квартире на 2 000 кв. футов с максимальной дистанцией взгляда ≈ 15 м **HLOD не нужен**: он даёт выигрыш от 100 м и дальше. Он понадобится только для сценария «целый этаж из коридора» в будущем building-режиме.

## 5. Materials

Семь мастер-материалов покрывают всю квартиру; всё остальное — Material Instances. Общие параметры во всех мастерах одинаковы (`UVScale`, `UVRotation`, `ColorTint`, `RoughnessMin/Max`, `NormalStrength`, `DetailNormal`, `DetailTiling`), чтобы конфигуратор и артисты не учили семь интерфейсов.

| Master | Что покрывает | Особенности графа |
| --- | --- | --- |
| M\_Master\_Surface | Краска (PT01 White Dove, PT02 Chantilly Lace, PT03 Iron Mountain), ламинат (LAM01–LAM06), limewash (PT04/PT05), гипс, плитка | BC/N/ORM + detail normal + MF\_RoughnessBreakup (шум 2 октавы) + опция Metallic для LAM02 Cashmere Gold |
| M\_Master\_Wood | M01 rift sawn white oak, дуб/орех конфигуратора, мебель | См. ниже |
| M\_Master\_Stone | SS01 Neolith Estatuario, SS02 Cipollino, SS03 белый мрамор, плитка пола TL01 | См. ниже |
| M\_Master\_Glass | GL02 bronze glass, стекло витрин, GL04 rippled den door, окна | Translucent, Surface ForwardShading |
| M\_Master\_Mirror | GL01 bronze mirror, GL03 clear mirror | Opaque, Metallic 1, Roughness 0.02, Tint |
| M\_Master\_Metal | HW01 polished nickel, PF luxe gold, MT01 bronze, MT02 matte black, stainless | Anisotropy, brushed direction |
| M\_Master\_Fabric | FB01 Midori ivory, FB02 Cry beige, FB03 Bellows taupe, букле, ковёр | Sheen/fuzz, subsurface для штор |
| M\_Master\_Emissive | LED-лента L02, рассеиватели | Emissive в nit, параметр Intensity управляется из BP\_InteractiveLight |

### Wood — M\_Master\_Wood

Параметры: BaseColor, Normal, Roughness, AO, Height (для pixel-depth на hero), GrainDirection (0/90°), UVScale, RoughnessMin / RoughnessMax, MicroNormal + MicroTiling, ColorTint, Desaturation, Sheen (для матового масла).

| Уровень | Текстуры | Значения |
| --- | --- | --- |
| WOOD HERO (M01 панели столовой и гостиной, фасады в кадре крупно) | 4K BC, 4K N, 4K R, 2K AO, 2K H; физический размер тайла 120×120 см (рифт-дуб имеет прямое волокно, повтор менее заметен) | Roughness 0.38 ± 0.08, MicroNormal tiling 25 см, strength 0.35; Normal strength 0.6 (шпон почти плоский) |
| WOOD STANDARD (мебель, полки) | 2K BC/N/R, 1K AO, тайл 100 см | Roughness 0.4 ± 0.06 |
| WOOD BACKGROUND (внутренности шкафов — но их видно при открытии) | 2K BC, 1K N/R, ламинат LAM01 через M\_Master\_Surface | Roughness 0.55 (soft touch) |

Почему не пластик: вариация roughness по волокну (ранняя древесина глаже поздней) берётся из R-карты, а не константой; микро-нормаль даёт блик на просвет; фаска 1 мм + Lumen = свет на ребре. Для панелей массивом (стена столовой 16 футов) — world-aligned проекция с 2 вариантами текстуры и MF\_TileBreaker, чтобы повтор не читался.

### Stone — M\_Master\_Stone

| Поверхность | Текстура | Физический масштаб | Значения |
| --- | --- | --- | --- |
| Столешница кухни + waterfall (SS01 Estatuario 63×126") | 8K BC (скан слэба или фото поставщика), 4K N, 4K R, Streaming VT | 1 тайл = 1 слэб 160×320 см, UV острова и waterfall развёрнуты совместно — жила переходит через ребро | Roughness 0.18 (silk) ± 0.04; macro variation 0; micro roughness tiling 10 см, 0.03 |
| Фертурная стена столовой (SS02 Cipollino 6 мм, синий камень) | 8K BC, 4K N, 4K R | Один слэб на панель, bookmatch — зеркальный UV соседней панели | Roughness 0.12 (polished) |
| Плитка пола TL01 (существующая, нейтральный травертин-вид) | 4K BC/N/R с 3–4 плитками в тайле | Тайл 240×240 см, шов 1.5 мм в геометрии normal | Roughness 0.45 (matte), macro variation 0.15 через шум 8 м |
| Столешницы ванных (SS03) | 4K | Слэб 120×240 см | Roughness 0.1 polished / 0.35 honed |

8K оправдан только там, где один слэб занимает весь экран в photo mode (остров, стена столовой): 320 см / 8192 px = 0.4 мм/px, в 4K было бы 0.8 мм/px — видно на стилле 6K.

### Glass — M\_Master\_Glass

| Тип | Blend | Параметры |
| --- | --- | --- |
| Clear glass (окна, витрины) | Translucent, Surface ForwardShading, Lumen reflections ON (`r.Lumen.TranslucencyReflections.FrontLayer.Enable=1`) | Opacity 0.08, Roughness 0.02, Specular 0.5, Refraction 1.0 (отключена — в интерьере рефракция плоского стекла незаметна, а артефакты заметны) |
| Bronze glass GL02 (витрина кухни) | то же | Opacity 0.35, Tint (0.45, 0.33, 0.22), Roughness 0.03 |
| Smoked / tinted | то же | Opacity 0.5, Tint нейтральный тёмный |
| Fluted / rippled GL04 (дверь den) | то же + Normal рифления (tiling 2 см) + Refraction 1.33 включена | Opacity 0.4, Roughness 0.08 — здесь рефракция и есть эффект; PDLC-режим = параметр Opacity 0.95 + Roughness 0.6 |

Стеклянные фасады витрины: стекло — отдельный меш с реальной толщиной 6 мм, обе стороны (Two Sided OFF, две плоскости), чтобы блик был двойной — это то, что отличает стекло от «прозрачного пластика».

### Mirror

Три уровня зеркал. (1) **Walkthrough** — Lumen reflections: размытое, неполное отражение (Surface Cache + screen traces); для бронзового GL01 в фойе и столовой тонировка скрывает неточность, для GL03 в ванных и WIC принимаем размытость. (2) **Вблизи hero-зеркала** (trigger < 150 см) — локальный quality boost: Lumen Reflection Quality 2, `r.Lumen.Reflections.DownsampleFactor 1`, MaxRoughnessToTrace 0.6. (3) **Photo / Cinematic mode** — `Planar Reflection` actor на плоскости зеркала (второй рендер сцены, ≈ 3–5 мс на RTX 4080, 8–12 мс на M2 Max — в обычном walkthrough не включается ни на одном профиле). Scene Capture 2D не используем: тот же cost без корректной перспективы. На Windows RTX с HWRT Lumen даёт точное зеркало и уровни 2–3 не нужны.

### Metal — M\_Master\_Metal

| Финиш | Metallic / Roughness | Anisotropy | Детали |
| --- | --- | --- | --- |
| Black anodized aluminum (MT02 рама двери den) | 1 / 0.45 | 0 | Micro scratches 0.2, roughness breakup tiling 15 см |
| Brushed aluminum / brushed stainless (существующие ручки) | 1 / 0.35 | 0.6, направление по tangent | Brushed normal tiling 5 см |
| Polished nickel (HW01 Emtek) | 1 / 0.08 | 0 | Tint слегка тёплый (0.9, 0.88, 0.84) |
| Brass / luxe gold (Brizo PF02–PF13), aged bronze (Gessi PF01) | 1 / 0.25 (luxe gold), 0.4 (aged bronze) | 0.3 | BC gold (1.0, 0.77, 0.4); aged bronze (0.45, 0.33, 0.2) + пятна патины 0.15 |
| Bronze MT01/MT04 (витрина, столовая) | 1 / 0.3 | 0.2 | Совпадает с LAM02 по тону |
| Stainless (техника) | 1 / 0.3 | 0.7 | Fingerprint mask 0.1 |

### Fabric — M\_Master\_Fabric

Shading model **Cloth** (fuzz color + cloth 0.6) для букле и бархата; **Subsurface** для штор FB01/FB02 (лён/полиэстер, свет из окна просвечивает). Normal плетения tiling 4–6 см, roughness 0.8–0.95, sheen tint = BC × 1.3. Ковёр — тот же мастер + Height для parallax 0.5 см. Проект идёт на **Substrate Blendable GBuffer** (default 5.8, parity с legacy): мастера пишутся в Substrate-нодах, но с одним slab — простыми. Многослойный бархат (fuzz + base) через Adaptive GBuffer — опциональный hero-профиль только после замера на прототипе, не по умолчанию.

### Texture strategy

| Класс | Разрешение | Формат | Примеры |
| --- | --- | --- | --- |
| Hero | 8K BC (VT), 4K N/R | BC8 / BC5 / BC4 | Слэбы острова и столовой, панели M01 |
| Standard | 4K BC, 2K N/R, 2K AO | то же | Фасады, мебель, пол |
| Background | 2K BC, 1K N/R | то же | Внутренности шкафов, потолок, декор дальше 3 м |
| Detail normals | 1K tiling, общие на проект | BC5 | 6 штук: wood micro, paint orange peel, stone micro, fabric weave, brushed metal, leather |

ORM-паковка (AO/Roughness/Metallic в одной текстуре) для Standard и Background; Hero держит R отдельно для точности. Streaming Virtual Textures включены для всего ≥ 4K; mip bias в scalability LOW = +1. Бюджет VRAM под текстуры — в разделе 13.

## 6. Lighting, Lumen, shadows, exposure, post process

Рецепт под референс: одно солнце + небо дают основной свет днём, интерьерные светильники горят всегда (как в дизайнерском рендере), Lumen переносит тёплый свет от дерева и камня на белые плоскости. Все значения в физических единицах (lux, lm, cd, EV100).

### Daylight (Dempster: окна на восток — Intracoastal, балкон вдоль всего фасада)

| Актор | DAY | EVENING | NIGHT |
| --- | --- | --- | --- |
| Directional Light (Sun) | 75 000 lux, 5 500 K, высота 38°, азимут под углом 30° к окнам (свет падает на пол гостиной, не в камеру), Source Angle 0.54°, VSM ON, Cast Cloud Shadows OFF | 8 000 lux, 3 200 K, высота 6° | 0 (луна не нужна) |
| Sky Atmosphere | ON, Rayleigh по умолчанию | ON | ON |
| Sky Light | Real Time Capture ON, Intensity 1.0, Lower Hemisphere Is Solid Color OFF | 1.0 | 0.15, цвет неба города (сине-серый) |
| Volumetric Clouds | OFF (из окна видна вода и горизонт; HDRI бэкдроп дешевле) | OFF | OFF |
| Exterior backdrop | HDRI Backdrop actor, 8K HDRI набережной, только визуал (Lighting OFF) | вечерний HDRI | ночной HDRI |

Окна не выбеливаются за счёт трёх вещей: солнце не смотрит в камеру, стекло окон имеет Tint 0.75 (реальный стеклопакет кондо во Флориде — тонированный low-E), и Local Exposure (ниже) поджимает света на 1.5 стопа.

### Interior lighting (по спецификации D0.03, всё на диммерах)

| Тип | Источник в UE | Интенсивность | Температура | IES | Тени |
| --- | --- | --- | --- | --- | --- |
| Recessed can lights L01/L03 (существующие, квадратные) | Spot Light | 700 lm (≈ 9 W LED), cone 25/45° | 3000 K | IES\_Downlight\_36deg — да | VSM, только ближайшие 8 к камере (Max Draw Distance 1 200 см) |
| Directional pot lights (над картинами, стеной столовой) | Spot Light | 500 lm, cone 15/30° | 2700 K | IES\_Spot\_24deg — да | VSM |
| Linear LED L02 (undercabinet, cove, полки витрины, WIC) | Rect Light 1 на каждые 120 см + emissive mesh ленты | 160 lm/ft → на 120 см ≈ 630 lm, Rect 120×1 см, Barn door 80° | 3000 K | нет (Rect сам даёт правильный распад) | OFF для cove, ON для undercab |
| Cabinet LED (внутри шкафов, включается при открытии) | Rect Light + emissive | 300 lm | 3000 K | нет | OFF |
| Decorative pendant (люстра столовой L07 — кольцо, гостиная L05 — «облака») | Point Light в центре + emissive рассеиватели | 1 500–2 500 lm суммарно, Source Radius 15 см | 2700 K | нет | VSM ON (главная тень на столе) |
| Wall sconce (фойе L04, спальни L09/L12) | Point Light + emissive плафон | 400 lm | 2700 K | нет | ON |
| Accent (ниши, стена фойе с плетёным паттерном) | Spot Light | 300 lm, cone 10/40° | 2700 K | IES\_WallWasher — да | OFF |

Когда IES: только там, где пятно света видно на стене (downlight у стены, акцент на картине, wall washer). Для downlight над серединой комнаты и для всех линейных источников обычный Spot/Rect дешевле и неотличим.

Бюджет: ≤ 12 теневых источников в кадре, ≤ 60 локальных светов в кадре; все локальные света — Movable (нужно для выключателей), Attenuation Radius минимально достаточный (downlight 350 см, LED 150 см).

### LED — BP\_InteractiveLight

Один актор = одна лента: child-компоненты `StripMesh` (материал M\_Master\_Emissive, Dynamic Material Instance), массив `RectLights`, параметры `LightGroupTag` (Gameplay Tag), `Lumens`, `Kelvin`, `EmissiveNits` (реальная лента с опаловым рассеивателем ≈ 3 000 nit → emissive 15 при EV 10). Состояния ON/OFF с Timeline 0.3 с (диммер не щёлкает). Emissive сам по себе в Lumen светит слабо и шумно — поэтому Rect Light обязателен, а emissive даёт только видимую полоску и блик в столешнице.

### Lumen (REAL-TIME WALKTHROUGH, NOAR\_HIGH)

```ini
r.Lumen.ScreenProbeGather.RadianceCache.ProbeResolution=32
r.Lumen.ScreenProbeGather.DownsampleFactor=16      ; 8 на ULTRA
r.Lumen.ScreenProbeGather.TracingOctahedronResolution=8
r.Lumen.ScreenProbeGather.TemporalFilterProbes=1
r.Lumen.Reflections.Allow=1
r.Lumen.Reflections.DownsampleFactor=1             ; 2 на Mac
r.Lumen.Reflections.MaxRoughnessToTrace=0.4
r.Lumen.Reflections.SmoothThreshold=0.1
r.Lumen.TraceMeshSDFs=1                            ; Detail tracing — фаски и мелкая мебель
r.Lumen.TraceMeshSDFs.TraceDistance=180            ; маленькая квартира — ближний радиус
r.LumenScene.Radiosity.ProbeSpacing=4
r.LumenScene.SurfaceCache.CardMinResolution=4
r.Lumen.MaxTraceDistance=2000                      ; вся квартира — 20 м
r.Lumen.Reflections.HardwareRayTracing=1           ; только Windows RTX profile
r.Lumen.HardwareRayTracing.LightingMode=2          ; Hit lighting для зеркал, ULTRA RTX
```

Post Process Volume: Lumen Scene Lighting Quality 1 (2 на ULTRA), Scene Detail 1, Final Gather Quality 1 (2 на ULTRA), Max Trace Distance 2000, Reflection Quality 1, Ray Lighting Mode = Surface Cache (Hit Lighting только HWRT). **Software vs Hardware**: на Mac только Software (факт, не настройка); на RTX HWRT даёт точные отражения в стекле/зеркалах и точные тени от GI за +2–3 мс. Общий вид отличается мало — вся архитектура проекта строится под Software Lumen, HWRT — бонус профиля.

**Lumen Lite (5.8) для NOAR\_MEDIUM / Mac**: Medium GI использует Irradiance Final Gather (`r.Lumen.FinalGatherMethod 0`), Medium Reflections отключает Lumen Reflections и даёт SSR на гладких поверхностях + rough specular из GI. Epic: каждый уровень ниже стоит примерно вдвое дешевле — но для нашей сцены это измеряется на prototype room, а не фиксируется как гарантия. Следствие SSR: отражения в витрине и на Neolith становятся экранно-зависимыми (пропадают у краёв кадра) — для walkthrough приемлемо, photo mode на Mac переключает отражения на High на время кадра.

**MegaLights (Production Ready в 5.8)** — отдельный профиль `NOAR_ULTRA_RTX` в v1.1: интерьер с 60+ Rect/Spot/IES — прямой кейс, бенчмарк на LS\_PerfRoute против VSM-схемы. На Apple Silicon M2+ HWRT Lumen и MegaLights имеют experimental-статус — профиль `NOAR_MAC_EXPERIMENTAL` только для тестов, в клиентский билд не попадает (отдельный ini, не в DefaultScalability).

**Ground Truth Render Gate**: раз в milestone один и тот же hero-кадр (остров кухни с окном) рендерится Path Tracer-ом (MRQ, 2048 spp) и кладётся рядом с Lumen real-time: расхождение по яркости bounce, цвету теней и отражениям — критерий GATE 1.

### Virtual Shadow Maps

```ini
r.Shadow.Virtual.ResolutionLodBiasDirectional=-0.5  ; солнце резче
r.Shadow.Virtual.ResolutionLodBiasLocal=0.5         ; локальные мягче и дешевле
r.Shadow.Virtual.SMRT.RayCountDirectional=8
r.Shadow.Virtual.SMRT.SamplesPerRayDirectional=4
r.Shadow.Virtual.SMRT.RayCountLocal=6
r.Shadow.Virtual.SMRT.SamplesPerRayLocal=3
r.Shadow.Virtual.Cache.StaticSeparate=1             ; статика кэшируется, двигаются только фасады и персонаж
```

Source Radius солнца 0.54°, Point/Spot Source Radius 3–8 см, Rect — реальный размер: тени мягкие у основания и растут с расстоянием, как в фотографии. Акторы мебели — Static mobility (кэш VSM), интерактивные фасады — Movable, но не инвалидируют кэш пока не двигаются.

### Exposure

| Параметр | Значение | Зачем |
| --- | --- | --- |
| Metering | Auto Exposure Histogram | Естественная адаптация окно → closet → гостиная |
| Min / Max EV100 | 7.5 / 12.5 | Не даёт выбелить окно и не даёт «ночной режим» в closet |
| Exposure Compensation | +0.3 | Чуть светлее нейтрали, как дизайнерский рендер |
| Speed Up / Down | 2.0 / 1.0 | При повороте к окну глаз адаптируется быстрее, в тёмное — медленнее |
| Histogram Low / High | 70 / 95 % | Игнорируем окна в метринге |
| Local Exposure | Highlight contrast 0.65, Shadow contrast 0.8, Detail strength 1.0, Blurred luminance blend 0.6 | Окна сохраняют детали без HDR-вида |
| Photo mode / стиллы | Manual, EV100 задаёт пользователь (default 10.5) | Повторяемость кадра |

### PP\_NOAR\_MASTER (Unbound, priority 0; комнатные волюмы priority 1 меняют только Exposure Compensation)

| Блок | Значение |
| --- | --- |
| White Balance | Temp 6 000 K (сцена с лампами 3 000 K становится тёплой, но белые стены White Dove остаются белыми), Tint 0 |
| Global | Saturation 1.05, Contrast 1.03, Gamma 1.0, Gain 1.0 |
| Shadows | Saturation 0.95, Gain 1.04 (не давить чёрный металл), Shadows Max 0.09 |
| Highlights | Saturation 0.92, Gain 0.97, Highlights Min 0.5 |
| Tone curve | Filmic: Slope 0.88, Toe 0.5, Shoulder 0.26, Black clip 0, White clip 0.04 |
| Bloom | Standard, Intensity 0.35, Threshold 1.5, Size scale 3 |
| Vignette | 0.25 |
| Chromatic Aberration | 0 |
| Motion Blur | Amount 0.25, Max 3 %, Target FPS 30; Photo mode 0 |
| Film Grain | 0.1 (только в photo mode и MRQ) |
| Lens flare | 0 |
| Sharpen (r.Tonemapper.Sharpen) | 0.3 |
| Ambient Occlusion | выключен (Lumen даёт свой) |

Чего избегаем: Bloom > 0.5, Contrast > 1.1, Temp > 7 000 K (оранжевый тинт), Toe > 0.6 (crushed blacks), любой LUT из «кино»-паков. Цель — нейтральная интерьерная фотография: белые стены белые, дуб тёплый, Cipollino сохраняет синий, чёрная рама den читается как металл, а не как дыра.

## 7. Character и camera

`BP_NOAR_Character` наследуется от Character шаблона, но все «игровые» входы (Jump, Sprint, Crouch) удалены из IMC. Остаются Move, Look, Interact, ToggleCamera, PhotoMode, Menu.

### Character Movement

| Параметр | Шаблон (= референс) | NOAR | Почему |
| --- | --- | --- | --- |
| Max Walk Speed | 500 | 160 (Shift: 280) | Человек дома идёт 1.2–1.6 м/с; в референсе бег — его главный недостаток |
| Max Acceleration | 2048 | 600 | Плавный старт 0.3 с |
| Braking Deceleration Walking | 2048 | 900 | Остановка за ≈ 20 см, без скольжения и без рывка |
| Ground Friction | 8 | 8 | — |
| Braking Friction Factor | 2 | 1.5 | Мягче торможение |
| Rotation Rate (Yaw) | 500 | 360 | Разворот на 180° за 0.5 с |
| Use Controller Rotation Yaw | OFF | OFF (3P) / ON (1P) | В 3P камера свободна |
| Orient Rotation to Movement | ON | ON (3P) / OFF (1P) | Как в референсе |
| Capsule radius / half height | 35 / 90 | 30 / 90 | Проходит между стульями и островом (проход 42") |
| Step height | 45 | 15 | Не залезает на мебель |
| Can Jump / Crouch | ON | OFF | — |

### Camera — NOAR third person

| Параметр | Шаблон | NOAR |
| --- | --- | --- |
| Target Arm Length | 400 | 320 |
| Socket Offset | (0, 0, 0) | (0, 35, 55) — персонаж чуть левее центра, вид сверху мягче |
| Target Offset | (0, 0, 0) | (0, 0, 40) — точка вращения на уровне груди |
| FOV | 90 | 75 (слайдер в Settings 65–85) |
| Camera Lag | OFF | ON, speed 8, Max Lag Distance 60 |
| Camera Rotation Lag | OFF | ON, speed 12 |
| Pitch limits (PlayerCameraManager) | −89 / +89 | −60 / +25 |
| Look sensitivity | 1.0 | 0.6, плюс экспоненциальное сглаживание input (interp speed 15) |
| Do Collision Test | ON, probe 12 | ON, probe 14, channel `Camera`; стекло и тонкий декор — Ignore |
| Collision recovery | мгновенно | Кастом: длина руки интерполируется (FInterpTo 6) — без «прыжка» камеры у стены |
| Auto-zoom в узких местах | нет | Trigger volumes `BPV_CameraZone` (коридор, WIC, ванные): Arm 200, FOV 80 с блендом 0.8 с |
| Fade персонажа | нет | При Arm < 120 — персонаж dither-fade до 0 (не закрывает фасад) |

### First person (V)

Вторая камера `FP_Camera` на сокете head, высота глаз 165 см, FOV 80, Use Pawn Control Rotation ON. Переключение — `SetViewTargetWithBlend` 0.6 с, EaseInOut 2.0; на время бленда ввод движения заморожен. В 1P скелет скрыт (Owner No See), скорость 140, head-bob отсутствует (добавляет тошноту и ничего не даёт презентации). Автоматически предлагается (подсказка «V — first person») при входе в WIC, ванные, powder room; никогда не переключается без действия пользователя. В 1P взаимодействие идёт по центру экрана (точка-ретикл), в 3P — по курсору (раздел 8).

### Анимация и персонаж

`ABP_NOAR_Walker`: State Machine Idle → Walk (BS\_1D по Speed: 0 Idle, 160 Walk, 280 FastWalk) → Idle; состояние TurnInPlace при |Δyaw| > 60° без движения (две анимации L/R 90). Inertialization blend 0.25 с на всех переходах, Leg IK на пол (Control Rig из шаблона) — больше ничего: персонаж в кадре 20 % времени и со спины.

| Вариант | Плюсы | Минусы | Решение |
| --- | --- | --- | --- |
| UE Mannequin (Manny/Quinn) | Бесплатно, готовый ABP, референс им и сделан | Выглядит как робот, спорит с интерьером | MVP и Gate 1–4 — Manny |
| MetaHuman | Фотореализм, тот же скелет — анимации Manny подходят | Лицо в uncanny valley со спины не видно, а стоит 2–4 мс и 8 странд-грумов волос; на Mac groom тяжёл | Решение Manny vs MetaHuman vs custom — на GATE 2, не после полной квартиры (персонаж меняет GPU-бюджет, тени и camera feel); если MetaHuman — LOD1, волосы-карты (не groom), нейтральная одежда беж/серый |
| Mixamo | Быстро, много анимаций | Скелет другой (retarget), качество моделей ниже MetaHuman | Только как источник анимаций (ретаргет через IK Retargeter) |
| Custom character | Полный контроль стиля (силуэт без лица) | 2–4 недели работы | Вариант для бренда NOAR в v2 |

Важное отличие от референса: там персонаж — случайность шаблона. У NOAR персонаж — масштабная линейка (клиент видит высоту острова и глубину WIC относительно человека 180 см), поэтому рост фиксирован и показывается в Settings.

## 8. Interaction framework

Один компонент на персонаже, один интерфейс на всех объектах, Gameplay Tags для состояний и зависимостей. Ни один шкаф не имеет своей логики — только значения параметров.

### BPI\_Interactable

| Функция | Вход / выход | Назначение |
| --- | --- | --- |
| CanInteract | Instigator → bool, Reason (enum: OK, Locked, Blocked, TooFar, Dependency) | Спрашивается каждый кадр при фокусе |
| Focus / Unfocus | — | Включает highlight, подсказку |
| Interact | Instigator | Основное действие (toggle) |
| GetInteractionText | → Text | «Open» / «Close» / «Lights» / «Switch finish» — локализуемый |
| GetInteractionPoint | → Vector | Где рисовать точку-индикатор (ручка фасада, клавиша выключателя) |
| GetProductInfo | → DA\_Product (может быть null) | Для панели с производителем/ценой в будущем |

### BPC\_Interaction (на BP\_NOAR\_Character)

Поток каждый кадр (фактически раз в 0.05 с по таймеру):

1. **Trace**. В 1P — Line Trace из камеры по центру; в 3P — Line Trace под курсором (`GetHitResultUnderCursorByChannel`, канал `Interactable`). Длина 600 см. Sphere Trace не нужен: фасады крупные, а сфера попадала бы в соседний ящик.
2. **Distance check**: расстояние от капсулы персонажа (не от камеры) до точки попадания ≤ **MaxInteractionDistance = 200 см**. 150 мало (не дотянуться до верхнего шкафа через остров), 250 много (открывается через комнату). Дальше — индикатор виден, но полупрозрачный; поведение клика зависит от режима: Mouse 3P → click-to-walk разрешён (Simple Move To, открывает по приходе); First Person → подсказка «Move closer»; Gamepad → «Move closer».
3. **Occlusion**: второй Line Trace от груди персонажа к GetInteractionPoint по каналу `Visibility`; если первый hit — не этот актор и не его родитель-корпус, объект заслонён (ящик за закрытой дверью). Закрытые фасады блокируют Visibility, открытые — нет.
4. **Dependency**: актор имеет массив `RequiredOpen` (soft refs на другие interactables). CanInteract возвращает Dependency, пока хотя бы один не в состоянии Open; подсказка «Open the door first». Обратно: корпус не даёт закрыть дверь, пока внутренний ящик открыт (сначала автозакрывает ящик, потом дверь — как сделал бы человек).
5. **Collision between moving parts**: логические `BlockedBy` (ящик под открытой дверцой соседнего шкафа, дверь холодильника рядом с выдвинутым ящиком) задаются при сборке в EUW\_MillworkAssembler автоматически: swept box траектории каждого фасада проверяется на пересечение с траекториями соседей один раз в эдиторе, а не физикой в runtime. В runtime — только проверка тегов.
6. **Focus**: если актор сменился — Unfocus старому, Focus новому; WBP\_InteractionPrompt получает текст и экранную позицию точки.
7. **Input**: E или LMB → Interact на текущем фокусе; повторное нажатие — обратное действие. Нажатие во время анимации разворачивает Timeline с текущей позиции (Reverse), а не игнорируется.

### Mouse vs camera

В 3P курсор всегда виден, камера вращается только с зажатой RMB (как в архитектурных вьюверах и Twinmotion-презентациях); LMB — взаимодействие и UI. В 1P курсор скрыт, мышь вращает камеру напрямую, взаимодействие по центру; Esc/Tab показывает курсор для панели конфигуратора. Input Mode = Game And UI, Lock Mouse = Do Not Lock в 3P.

### Highlight

Custom Depth + Stencil (значение 1 = focus, 2 = locked) и пост-материал `PP_Highlight` в PP\_NOAR\_MASTER (Blendable, перед tonemapper). Эффект: подъём яркости поверхности на +12 % (множитель scene color 1.12 внутри маски) и краевой контур 1 px с альфой 0.35 цвета тёплого белого, fade-in 0.15 с. Никакого цветного неона. Material overlay отвергнут: ломает Nanite-батчинг и требует переключения материала.

### UX

- Индикатор: точка 6 px с кольцом 1 px, белая с альфой 0.6, в точке ручки; при фокусе растёт до 10 px, рядом текст «OPEN» (12 pt, tracking 0.2 em, без рамки). Индикаторы показываются только в радиусе 400 см и гаснут через 2 с покоя мыши, чтобы кадр был чистым для скриншотов.
- Первые 10 секунд: после START на экране три строки внизу («WASD — walk · Right mouse — look · Click — open»), исчезают после первого успешного действия каждого типа. Ближайший фасад к стартовой точке (кухня) получает мягкую пульсацию индикатора, пока не открыт хотя бы один.
- Конфигуратор открывается кликом на поверхность с тегом `Configurable.*` или клавишей Tab — панель справа шириной 280 px, камера не блокируется.

### Gameplay Tags (корни)

```text
Interact.State.Closed / Opening / Open / Closing
Interact.Lock.Locked / Dependency / Blocked
Interact.Type.Door / Drawer / Flap / Slide / Pocket / Switch / Appliance / Configurable
Zone.Foyer / Kitchen / Dining / Living / Primary / WIC / Ensuite / Guest / Den / Powder / Laundry / Balcony
LightGroup.<Zone>.Ambient / Accent / Undercab / Cove / Cabinet
Configurable.CabinetFinish / Countertop / Floor / WallPanel / Fabric / Paint
```

## 9. Doors, drawers, BLUM, appliances, light switches

Все подвижные элементы наследуют один базовый класс `BP_InteractiveBase` (реализует BPI\_Interactable, состояния, Timeline, звук, зависимости, сохранение состояния). Дочерние классы отличаются только тем, как Timeline Alpha (0–1) превращается в трансформ.

### Общая логика (BP\_InteractiveBase)

```text
Interact
  → CanInteract? (Locked / Dependency / Blocked → подсказка, выход)
  → Switch State
       Closed  → State=Opening, Timeline Play,    Sound(OpenStart)
       Open    → State=Closing, Timeline Reverse, Sound(CloseStart)
       Opening → State=Closing, Timeline Reverse (с текущей позиции)
       Closing → State=Opening, Timeline Play
  Timeline Update(Alpha) → ApplyAlpha(Alpha)   // виртуальная, переопределяется в дочерних
  Timeline Finished     → State = Open | Closed, Sound(SoftCloseEnd если Closed)
                        → OnStateChanged (Event Dispatcher) → LED шкафа, зависимые объекты, SaveGame
  AutoClose? → таймер AutoCloseDelay → Interact
```

Параметры базы: `InteractionEnabled`, `Locked`, `AutoClose` + `AutoCloseDelay` (default 0 = отключено), `OpenDuration`, `CloseDuration`, `Curve` (soft-close, ниже), `SoundSet` (DA), `RequiredOpen[]`, `BlockedBy[]`, `LinkedLight` (BP\_InteractiveLight, включается при Open), `SaveKey` (= имя актора).

### Soft-close кривая (Curve Float, одна на проект, время 0–1)

| t | Alpha | Касательная | Смысл |
| --- | --- | --- | --- |
| 0.00 | 0.00 | 2.2 | Быстрый старт — рука толкнула |
| 0.45 | 0.78 | 1.0 | Контролируемое движение |
| 0.80 | 0.96 | 0.25 | Вход в доводчик BLUMOTION |
| 1.00 | 1.00 | 0.0 | Последние 4 % за 20 % времени — беззвучный доход |

Открытие использует ту же кривую (на открытии BLUM тоже демпфирует конец хода). Длительности: hinged door 0.9 с open / 1.1 с close; drawer 0.7 / 0.9; AVENTOS 1.2 / 1.4; межкомнатная дверь 1.4 / 1.6; pocket 1.2; холодильник 1.0 / 1.3.

### BP\_InteractiveDoor

Параметры: `OpenAngle` (105 для CLIP top 110°, 90 для межкомнатных, 115 для холодильника), `ClosedAngle` (0), `HingeSide` (Left/Right → знак), `OpeningDirection` (Outward/Inward — для межкомнатных), `CollisionWhileMoving` (Ignore Pawn). ApplyAlpha: `SetRelativeRotation(Yaw = Lerp(ClosedAngle, OpenAngle × Sign, Alpha))` на DoorMesh. Ручка — child DoorMesh, едет вместе. Межкомнатные D.03–D.13 — тот же класс с `Locked` для mech room.

### BP\_InteractiveDrawer

Параметры: `TravelDistance` (глубина корпуса − 2 см для полного выдвижения LEGRABOX/MOVENTO; для TANDEM partial — 0.75 глубины), `TravelDirection` (локальная X), `OpenSpeed/CloseSpeed` (через Duration). ApplyAlpha: `SetRelativeLocation(X = TravelDistance × Alpha)`. Внутренний лоток (TRAY) — вложенный BP\_InteractiveDrawer с `RequiredOpen = [родитель]`. Органайзеры Richelieu (спецификация) — статичные меши внутри ящика, едут с ним.

### BLUM и сложные механизмы — что рационально

| Механизм | Класс | Способ | Почему |
| --- | --- | --- | --- |
| CLIP top hinge (все фасады) | BP\_InteractiveDoor | Timeline + Lerp rotation | Одна ось; смещение чашки учтено pivot-ом |
| TANDEM / MOVENTO / LEGRABOX / MERIVOBOX | BP\_InteractiveDrawer | Timeline + Lerp location | Прямолинейно; разница между системами — только вид боковин и TravelDistance |
| AVENTOS HK / HK top | BP\_InteractiveFlap | Timeline + rotation Y, одна ось | Рычаг — декоративный child, вращается по Look-at |
| AVENTOS HF (bi-fold вверх) | BP\_InteractiveFlap, mode BiFold | Два меша, две кривые в одном Timeline (Upper, Lower) | Кривые сняты с чертежа Blum: верхняя 0→−90°, нижняя относительно верхней 0→+170° |
| AVENTOS HS (swing up over cabinet) | BP\_InteractiveFlap, mode Swing | Timeline с 3 кривыми: Z-подъём, X-сдвиг, наклон | Траектория не осевая, но кривые дешевле и точнее физики |
| AVENTOS HL (parallel lift) | BP\_InteractiveFlap, mode Lift | Timeline 2 кривые: Z-подъём, небольшой X | То же |
| Pocket system (Hawa Concepta / Blum REVEGO) | BP\_InteractivePocket | Два последовательных Timeline: rotate 0→90°, затем translate в карман | Каретка — родитель, дверь — child |
| Sliding / pocket двери D.10, D.12, D.14 | BP\_InteractiveSlidingDoor | Timeline + Lerp location по треку | Для double sliding D.14 — два актора, один триггер |
| Murphy bed (den) | BP\_InteractiveFlap, mode Swing + child legs | Timeline + child Timeline ножек | — |
| Control Rig / Sequencer | — | Не используем для мебели | Control Rig оправдан только для IK-цепей (напр. пантограф стола), таких в квартире нет; Sequencer — для cinematics, не для интерактива |

Два режима кинематики, чтобы слово Digital Twin не обещало инженерной точности, которой нет. **VISUAL KINEMATICS** (95 % шкафов): одна ось или простые кривые из таблицы выше — CLIP top на самом деле четырёхзвенник, AVENTOS — рычаг со смещением, но на среднем плане разницы нет. **MANUFACTURER KINEMATICS** (hero-механизмы: витрина кухни, pocket-система, Murphy bed): траектории строятся по реальным данным Blum/Hawa (схемы сверлений и траекторий из каталога), два–три звена как вложенные компоненты с отдельными кривыми. Режим — поле `KinematicsMode` в BP\_InteractiveBase; по умолчанию Visual.

### Appliances

| Прибор | Класс | Pivot / угол | Внутренние части |
| --- | --- | --- | --- |
| Refrigerator / Freezer (интегрированный, панель LAM01) | BP\_InteractiveDoor | Петля кромки, 115° | Полки статичны, ящики овощей — BP\_InteractiveDrawer с зависимостью; свет внутри — LinkedLight |
| Dishwasher (панель) | BP\_InteractiveFlap, вниз | Нижняя кромка, 0→90° | Корзины — 2 × BP\_InteractiveDrawer с RequiredOpen |
| Oven / духовка (существующая колонна) | BP\_InteractiveFlap, вниз | Нижняя кромка, 0→90° | Противень — drawer с зависимостью; стекло дверцы — M\_Master\_Glass smoked |
| Microwave | BP\_InteractiveDoor или Flap по модели | 90° | — |
| Wine cooler (если в витрине) | BP\_InteractiveDoor | 110° | Полки — drawers |
| Washer / Dryer (laundry closet) | BP\_InteractiveDoor | Фронтальная петля, 160° | — |

### BP\_LightSwitch и Light Groups

`BP_LightSwitch` — interactable с мешем клавиши (анимация наклона 8°, 0.1 с) и массивом `LightGroupTags`. Interact → `BP_LightingManager::ToggleGroup(Tag)` → Event Dispatcher `OnGroupChanged(Tag, bOn, Intensity)`; каждый BP\_InteractiveLight подписан на менеджер на BeginPlay и реагирует, если тег совпадает (`MatchesTag` — поэтому «LightGroup.Kitchen» гасит и Undercab, и Cove). Ни один выключатель не знает конкретных светильников — свет можно переставлять без правки логики. Та же функция доступна из UI (список зон) и из BP\_DayNightManager (вечер включает Accent и Cove на 60 %, ночь — всё на сценарий «evening» с Ambient 30 %).

### Звук

`DA_SoundSet` на тип: OpenStart, CloseStart, SoftCloseEnd (отдельные сэмплы для ламинатного фасада, шпона, стекла в раме, металла LEGRABOX, межкомнатной двери, холодильника). Громкость −18 dB относительно ambience, attenuation 50–400 см, никаких «щелчков»: soft-close — это тихий выдох демпфера и касание.

## 10. Material configurator, data architecture, save, day/night, teleport

Всё data-driven: блупринты не знают ни одного материала, комнаты или цены по имени. Добавить финиш = создать один Data Asset.

### Data assets

| Asset | Хранит | Кто читает |
| --- | --- | --- |
| DA\_Apartment | ID, название, адрес, уровень (soft ref), список DA\_Room, DT\_FinishOptions, DA\_LightPreset\[\] (Day/Evening/Night), стартовая точка, превью-картинка; регистрируется как Primary Asset (Asset Manager) уже в v1.0 — Primary Asset Labels и per-apartment chunks добавляются при второй квартире без смены архитектуры, версия дизайна («Сoncept 08.11.2026») | BP\_ApartmentManager, WBP\_MainMenu, будущий apartment selector |
| DA\_Room | Имя, Zone tag, точка телепорта (transform), рекомендуемый режим камеры (3P/1P), превью, список Configurable-слотов комнаты, Light groups | WBP\_RoomNavigation, конфигуратор (фильтр опций по комнате) |
| DA\_MaterialOption | Name (локализуемый), Thumbnail, MaterialInstance (soft), Category (тег Configurable.\*), SKU, Manufacturer, Price (per unit + unit), Description, совместимость (напр. только для столешниц) | BP\_MaterialConfigurator, WBP\_ConfigPanel |
| DT\_FinishOptions (на квартиру) | Строка = Slot (Configurable.Countertop.Kitchen) → список DA\_MaterialOption + Default | Конфигуратор |
| DA\_Product | Manufacturer, Model, Material/Finish/Color, Price, URL, фото | GetProductInfo у любого interactable и мебели |
| DA\_LightPreset | Для каждого LightGroup: on/off, intensity scale; Sun: lux, угол, K; Sky intensity; HDRI; Exposure compensation; WB | BP\_DayNightManager |
| DA\_SoundSet | Сэмплы по типу механизма | BP\_InteractiveBase |

### BP\_MaterialConfigurator

Слот = группа поверхностей, которые меняются вместе (все фасады кухни; столешница + waterfall + фартук; пол всей квартиры). Каждый меш декларирует участие компонентом `BPC_ConfigurableSurface` (SlotTag + имя material slot на меше). На BeginPlay конфигуратор собирает реестр `Map<SlotTag, Array<Component+SlotName>>` (один раз, ≈ 2 мс на 300 компонентов).

```text
ApplyOption(SlotTag, DA_MaterialOption)
  → Load MI (async, обычно уже в памяти: все опции квартиры предзагружены на loading screen)
  → для каждого компонента слота: SetMaterialByName(SlotName, MI)
  → CurrentConfig[SlotTag] = Option.ID
  → OnConfigChanged (dispatcher) → UI обновляет активную плитку и сумму (если цены включены)
  → SaveGame (debounce 1 с)
```

Смена мгновенна (один кадр), Lumen подхватывает новый цвет bounce за 0.3–0.5 с — это выглядит как плавное переосвещение и работает на презентацию. Для слотов с разным UV-масштабом (плитка 60×120 vs доска 20×220) опция несёт свою MI с правильным UVScale; меши пола развёрнуты в метрах (1 UV = 1 м), поэтому любой материал ложится в масштабе.

### Слоты Dempster v1.0

| Слот | Опции (default первая) |
| --- | --- |
| Kitchen cabinet finish | Existing white (существующая), LAM01 Light Grey soft touch, M01 rift oak, LAM02 Cashmere Gold |
| Kitchen countertop | SS01 Neolith Estatuario, SS02 Cipollino, Quartz white, Porcelain grey |
| Wall panels (dining/living) | M01 rift oak stained to LAM04, light natural oak, walnut |
| Floor | Existing tile TL01, light oak plank, limestone |
| Dining feature stone | SS02 Cipollino, SS01 Estatuario |
| Fabric (sofa/chairs) | FB01 Midori ivory, taupe boucle, blue (как в precedent) |
| Paint (walls) | PT01 White Dove, PT05 limewash Moth, warm grey |

### Product information

Клик по объекту с DA\_Product при зажатом Alt (или кнопка «i» в подсказке) → `WBP_ProductCard`: производитель, модель, материал, цвет, цена. В v1.0 карточки заполняются из спецификации D0.01–D0.05 (Gessi, Brizo, Emtek, Kravet, Deccotec, Neolith), цены скрыты флагом `ShowPrices` в GameInstance. Позже DA\_Product можно генерировать из CSV/API поставщика через Editor Utility — структура не меняется.

### Save system

`BP_NOAR_GameInstance` держит в памяти: SelectedApartment, CurrentConfig (Map\<Tag, OptionID>), UserSettings (quality, FOV, sensitivity, language, ShowPrices), CameraMode, LastRoom. `SG_NOAR_ClientConfiguration` — слот на квартиру (`NOAR_<ApartmentID>`) + общий слот `NOAR_Settings`. Сохраняется: выбор финишей, состояния света по группам, режим Day/Evening/Night, последняя позиция и камера, графические настройки. Не сохраняются открытые двери и ящики — квартира при старте всегда «прибрана». Запись — async, при каждом изменении с debounce и при выходе. Версия схемы в сейве: при несовпадении — сброс к default, а не краш.

**RESET DESIGN**: кнопка в WBP\_ConfigPanel → подтверждение → для каждого слота ApplyOption(Default из DT\_FinishOptions), свет и режим дня — из DA\_LightPreset Day, сохранение. Default-конфигурация = то, что утвердил дизайнер.

### BP\_DayNightManager

Три пресета DA\_LightPreset. Переход — Timeline 2.5 с: Sun rotation/intensity/temperature, Sky Light intensity, HDRI кроссфейд (два backdrop-материала с параметром Blend), Exposure compensation в PP, затем группы света через BP\_LightingManager (вечер: включаются с задержкой 0.8 с — как будто кто-то включил). Клавиша N или кнопка в UI. Lumen не требует пересчёта — это главная причина отказа от запечённого света.

### Room teleport и floor plan

`WBP_RoomNavigation` — вертикальный список из DA\_Room (Foyer, Kitchen, Dining, Living, Primary, WIC, Ensuite, Guest, Den, Balcony). Клик → fade to black 0.3 с → SetActorLocationAndRotation в точку DA\_Room → камера сбрасывает lag (TeleportType) → fade in 0.4 с → если комната рекомендует 1P, подсказка V. Обычная ходьба остаётся. Interactive floor plan — полезен и дешёв: это тот же список, но кнопки положены поверх плана D1.00 (одна текстура 2K) с маркером позиции игрока; делаем в Phase 11, потому что клиенты читают план лучше, чем список названий.

## 11. UI, client mode, input, photo mode, audio

UI почти невидим: белый текст с альфой 0.85 без подложек, один шрифт (гротеск с широким трекингом для заголовков, тот же в regular для текста), линии 1 px с альфой 0.3, никаких иконок-кнопок крупнее 24 px. Все виджеты двуязычные (EN/RU через String Tables).

| Widget | Что содержит | Когда виден |
| --- | --- | --- |
| WBP\_Splash | Логотип NOAR, название квартиры, START EXPERIENCE, язык, ползунок загрузки (фон — статичный рендер из MRQ) | Старт |
| WBP\_HUD | Точки-индикаторы, подсказка первых 10 с, название комнаты в углу (появляется на 2 с при входе), 4 кнопки справа снизу: Rooms, Finishes, Day/Night, Photo | Всегда; кнопки гаснут до 30 % через 3 с покоя |
| WBP\_InteractionPrompt | Точка + текст OPEN/CLOSE/LIGHTS, причина блокировки | При фокусе |
| WBP\_ConfigPanel | Панель 280 px справа: слоты текущей комнаты, плитки опций 64×64 с названием, RESET DESIGN внизу | Tab / кнопка / клик по Configurable |
| WBP\_RoomNavigation | Список комнат или план | M / кнопка |
| WBP\_ProductCard | Карточка продукта | Alt+клик |
| WBP\_PhotoMode | Ползунки FOV, EV, roll, DOF (aperture, focus distance), пресеты камер комнаты, кнопка Capture, «hide UI» | F |
| WBP\_Settings | Quality preset (NOAR\_LOW…ULTRA), разрешение/фуллскрин, FOV, чувствительность, язык, рост персонажа, звук | Esc → Settings |
| WBP\_MainMenu (Esc) | Resume, Rooms, Finishes, Settings, Restart tour, Quit | Esc |
| WBP\_DebugMenu | FPS, frame time (Game/Draw/GPU), draw calls, Lumen/VSM/Nanite stats, память текстур, позиция, кнопки визуализаций | `  в Development; в Shipping класс исключён ( `#if !UE\_BUILD\_SHIPPING`в C++-обёртке или папка в`DirectoriesToNeverCook\`) |

### Client mode

Standalone запуск: `L_Boot` (пустой уровень с WBP\_Splash) → async load `L_DEMPSTER_Main` с прогрессом → предзагрузка всех MI конфигуратора и PSO cache (шейдеры компилируются до первого кадра, а не при первом повороте камеры — включён PSO precaching) → START EXPERIENCE → fade → персонаж в private vestibule лицом к гостиной. Никаких следов UE: сплэш движка заменён, иконка и имя приложения — NOAR, консоль отключена в Shipping.

### Input (Enhanced Input, IMC\_NOAR\_Walk / IMC\_NOAR\_UI / IMC\_NOAR\_Photo)

| Действие | Клавиатура / мышь | Геймпад |
| --- | --- | --- |
| Move | WASD / стрелки | Left stick |
| Look | RMB + мышь (3P), мышь (1P) | Right stick |
| Fast walk | Shift | Left stick click |
| Interact | E / LMB | A / Cross |
| Camera mode | V | Y / Triangle |
| Photo mode | F | Back / Share |
| Finishes | Tab | X / Square |
| Rooms | M | D-pad up |
| Day/Night | N | D-pad right |
| Menu | Esc | Start |
| Zoom (приблизить камеру в 3P) | колесо | триггеры |

Геймпад — бесплатно через Enhanced Input; для презентаций в офисе клиенту удобнее дать Xbox-контроллер, чем мышь. В геймпад-режиме взаимодействие идёт по ближайшему к центру экрана interactable в радиусе 200 см.

### Photo mode

F → пауза персонажа, камера отсоединяется (свободный полёт в радиусе 300 см, не сквозь стены), персонаж скрывается по желанию, UI скрывается по H. Capture: `HighResShot 2` (или 3840×2160 фиксированно) с TSR history reset и 8 кадрами накопления (камера статична, TSR сходится до чистого кадра), Motion blur 0, Film grain 0.1, сохранение в `Documents/NOAR/<Apartment>/<date>.png` + показ превью 2 с. Пресеты камер комнаты (из DA\_Room) — те же точки, что дизайнер выбрал для рендеров: клиент получает тот же ракурс в своей конфигурации.

### Audio

Один MetaSound `MS_Ambience`: room tone −40 dB (кондиционер далеко), у окон и балкона — Ambient Sound Actor с водой/ветром/дальним городом, attenuation 300–900 см, громче при открытой балконной двери (событие от BP\_InteractiveSlidingDoor). Шаги — очень тихие, по поверхности (Physical Material: tile, wood, carpet), −28 dB; в 1P ещё тише. Музыки нет. Всё через Sound Classes Master/Ambience/Interaction/UI с ползунками в Settings.

## 12. Cinematics и still renders

Один уровень обслуживает и walkthrough, и видео, и стиллы: разница — только в пресете Movie Render Queue, который поднимает качество выше real-time за счёт времени рендера.

### Sequencer workflow

- Секвенсы лежат в `Apartments/DEMPSTER/Cinematics/`: `LS_DEM_Hero_30s` (website), `LS_DEM_Reel_Kitchen_9x16`, `LS_DEM_Reel_Living_9x16`, `LS_DEM_Client_Tour_90s`. Каждый — субсеквенсы по комнатам, чтобы пересобирать монтаж без пересъёмки.
- Камера: Cine Camera Actor, сенсор Full Frame 36×24, объективы 20–35 мм (никогда шире 18 — интерьер «разъезжается»), высота 110–140 см, вертикали прямые (pitch 0, композиция сдвигом высоты, как shift-объектив), aperture f/4–f/8, focus на hero-объект.
- Движение: Camera Rig Rail для проходов (скорость 20–35 см/с, ease 1.5 с на концах), Camera Rig Crane для спуска с потолка к острову; никаких ручных ключей позиции — только Rail Position и Crane Pitch/Yaw.
- В секвенсе анимируются те же BP\_InteractiveDoor/Drawer через трек Event → Interact (движение с soft-close бесплатно), Day/Night — через трек BP\_DayNightManager.
- Reels 9:16: тот же секвенс с другим Cine Camera (сенсор повёрнут, 24×36) и композицией под вертикаль — не кроп 16:9.

### Movie Render Queue — пресеты

| Пресет | Разрешение | AA | Samples | Особенности | Вывод |
| --- | --- | --- | --- | --- | --- |
| MRQ\_Reel | 1080×1920, 30 fps | TSR off, MRQ AA | Spatial 1, Temporal 16 | Motion blur из shutter 180°, `r.Lumen.ScreenProbeGather.DownsampleFactor 8`, Reflections quality 2 | PNG 16-bit → ffmpeg H.265 |
| MRQ\_Hero\_4K | 3840×2160, 30 fps | то же | Spatial 2, Temporal 16 | + `r.Lumen.Reflections.HardwareRayTracing 1` на RTX, Hit Lighting | EXR 16-bit → DaVinci |
| MRQ\_Still\_4K | 3840×2160 | — | Spatial 8, Temporal 8 | Warm-up 32 кадра (Lumen сходится), Motion blur 0 | EXR + PNG |
| MRQ\_Still\_6K | 6144×3456 | — | Spatial 8, Temporal 8 | Tiled rendering 2×2, overlap 10 %, texture mip bias −1 | EXR |
| MRQ\_Still\_8K | 7680×4320 | — | Spatial 16, Temporal 8 | Tiled 3×3; только Windows RTX (VRAM) | EXR |

CVars для всех MRQ: `r.ScreenPercentage 100`, `r.Shadow.Virtual.ResolutionLodBiasDirectional -1.5`, `r.Shadow.Virtual.SMRT.RayCountDirectional 16`, `r.Lumen.ScreenProbeGather.TracingOctahedronResolution 16`, `r.Lumen.Reflections.DownsampleFactor 1`, `r.Tonemapper.Sharpen 0.5`, `r.TextureStreaming 0` (всё в памяти). Стилл 6K на RTX 4080 ≈ 25–40 с кадр; на M2 Max — 4K доступен (≈ 1.5–2 мин), 6K/8K — только через tiled с риском нехватки памяти на 32 ГБ.

Что даёт MRQ сверх viewport: суперсэмплинг убирает шум Lumen и гостинг TSR, правильный motion blur по shutter, тени без фликера, 16-bit для грейда. Это и есть замена отдельного рендера для проектов, где есть digital twin: каждый стилл совпадает с тем, что клиент видел в прогулке.

## 13. Performance, scalability, hardware profiles

Цель: **60 fps на CLIENT STANDARD PC в 1440p**, 30 fps locked — нижняя граница для Mac и ноутбуков. Выбор не «60 или 30», а фиксированный frame rate по профилю (`t.MaxFPS`) — плавающий fps ощущается хуже стабильных 30.

### Бюджет кадра (NOAR\_HIGH, 1440p, цель 16.6 мс)

| Статья | Бюджет | Как держать |
| --- | --- | --- |
| Nanite треугольники в кадре | до 15 M (исходники суммарно до 150 M на квартиру) | Nanite сам регулирует; фаски не считаются |
| Non-Nanite треугольники | ≤ 2 M | стекло, листья, персонаж |
| Draw calls | ≤ 2 500 | Nanite батчит; каждый interactable — отдельный актор, но Nanite-меши не добавляют draw call на меш |
| Локальные света в кадре | ≤ 60, теневых ≤ 12 | раздел 6 |
| Текстуры (streaming pool) | стартово 4 000 MB RTX / 2 500 MB Mac; итоговое значение — по профилю железа (4060 и 4090, M2 Max 32 и 96 GB — разные машины), проверка stat streaming | VT + мипы; всего текстур на диске ≤ 6 GB на квартиру |
| VRAM итого | ≤ 10 GB (RTX 4080 16 GB), ≤ 14 GB unified (Mac 32 GB) |  |
| GPU: Lumen GI | 3.5 мс | DownsampleFactor 16, probe res 32 |
| GPU: Lumen Reflections | 2.0 мс | MaxRoughnessToTrace 0.4 |
| GPU: VSM | 2.5 мс | кэш статики, радиусы света |
| GPU: Nanite + base pass | 3.0 мс |  |
| GPU: translucency (стекло, тюль) | 1.0 мс | ≤ 3 слоя перекрытия |
| GPU: post + TSR | 2.0 мс |  |
| GPU резерв | 2.6 мс | planar reflection в ванной, конфигуратор открыт |
| CPU Game thread | ≤ 5 мс | трейс раз в 0.05 с, Tick только у движущихся акторов (Timeline включает Tick, покой выключает) |

### Scalability: NOAR\_LOW / MEDIUM / HIGH / ULTRA (DefaultScalability.ini, свои группы)

| Параметр | LOW | MEDIUM | HIGH (client default) | ULTRA |
| --- | --- | --- | --- | --- |
| Screen percentage (TSR) | 58 | 67 | 100 | 100 |
| Lumen GI quality (sg) | 1, DownsampleFactor 32, probe 16 | Lumen Lite: Irradiance Final Gather (r.Lumen.FinalGatherMethod 0) | 3, DSF 16, probe 32, Detail tracing | 4, DSF 8, HWRT (если есть) |
| Lumen reflections | screen-space only, DSF 2 | Lumen Lite: SSR на гладких + rough specular из GI; photo mode → High | Lumen, DSF 1, roughness ≤ 0.4 | Lumen HWRT hit lighting |
| VSM | res bias +1.5, SMRT 4/2 | +0.5, 6/3 | 0 / −0.5 dir, 8/4 | −1, 16/4 |
| Теневые локальные света | 4 | 8 | 12 | 16 |
| Planar reflection | OFF | OFF | OFF; quality boost у hero-зеркал; Photo mode → ON | OFF (HWRT-зеркала точные); Photo mode → ON |
| Texture mip bias | +1 | 0 | 0 | 0 |
| Nanite | ON | ON | ON | ON |
| Motion blur | OFF | 0.25 | 0.25 | 0.25 |
| Target FPS | 30 | 30 | 60 | 60 |

Автовыбор при первом запуске по `Synth Benchmark` (GPU index): < 150 → LOW, 150–300 → MEDIUM, 300–550 → HIGH, > 550 → ULTRA. Клиент может поменять в Settings; выбор сохраняется.

### Hardware profiles

|  | DEV MAC (M2/M3 Max, 32–64 GB) | CLIENT STANDARD PC (RTX 4060–4070 / RX 7700, 16 GB RAM) | HIGH-END RTX PC (4080/4090, 32 GB) |
| --- | --- | --- | --- |
| Разрешение | 2560×1440 @ 67 % (внутреннее 1707×960) | 2560×1440 @ 100 % | 3840×2160 @ 100 % |
| Пресет | MEDIUM | HIGH | ULTRA |
| Lumen | Software, Detail tracing OFF | Software, Detail tracing ON | Hardware RT, Hit lighting |
| Reflections | Lumen DSF 2 | Lumen DSF 1 | Lumen HWRT, зеркала точные, planar OFF |
| Shadows | VSM, bias +0.5 | VSM | VSM bias −1 |
| Target FPS | 30 locked | 60 | 60 (4K) / 120 (1440p) |
| Роль | Разработка, лукдев, презентация в офисе | Клиентский билд | Финальные билды, MRQ, Pixel Streaming |

### Mac: что реально ограничено

| Технология | Статус на Apple Silicon (UE 5.5/5.6) | Что делаем |
| --- | --- | --- |
| Lumen Software | Работает, медленнее RTX 4080 в 2.5–3 раза | Профиль MEDIUM, 67 % |
| Lumen Hardware RT | Experimental на Apple Silicon M2+ (вместе с MegaLights), не production | Baseline Mac — Software Lumen; HWRT/MegaLights — профиль NOAR\_MAC\_EXPERIMENTAL только для тестов, не в клиентском билде |
| Nanite | Работает (SM6/Metal 3), masked дороже | Листья — non-Nanite |
| VSM | Работает | — |
| TSR | Работает | — |
| DLSS / FSR 3 frame gen | Нет (DLSS — NVIDIA; FSR3 плагин — Windows) | MetalFX не интегрирован — TSR |
| Substrate | Experimental, медленнее | OFF |
| MetaHuman groom (strands) | Работает, дорого | Волосы-карты |
| Pixel Streaming host | Поддерживается (Windows, Linux, Mac; на M-серии кодирование через VideoToolbox) | Production-хост всё равно Windows RTX (облачные GPU, HWRT) — это рекомендация, не ограничение UE |
| MRQ tiled 8K | Память ограничена | 4K на Mac, 6K/8K на RTX |
| Shader compile | В 2–3 раза дольше чем на 16-ядерном PC | DDC на NAS/облаке (раздел 15) |
| Packaging Windows | Невозможно с Mac | Windows build machine |

Вывод: на Mac делается 90 % работы (сборка, блупринты, материалы, свет), но в команде нужен один Windows RTX (4080 достаточно; 4090 даёт +30 % в MRQ и запас для 8K, но не меняет картинку) для: финальных Windows-билдов, MRQ 6K/8K, HWRT-отражений в видео, Pixel Streaming. Разница в визуале walkthrough между Mac MEDIUM и RTX ULTRA: чистота отражений в стекле/камне и разрешение; свет, материалы и интерактив одинаковы.

### Профилирование — порядок

1. `stat unit` — кто узкое место: Game, Draw или GPU. В этом проекте почти всегда GPU.
2. `ProfileGPU` (или GPU Visualizer) — строки Lumen\*, ShadowDepths/VSM, BasePass, Translucency, PostProcessing; сравнить с бюджетом выше.
3. `stat Lumen`, `stat Nanite`, `stat ShadowRendering`, `r.Shadow.Virtual.ShowStats 1` — количество страниц VSM (если кэш инвалидируется каждый кадр — кто-то Movable двигается без причины).
4. `stat streaming`, `stat RHI` — текстурный пул и VRAM.
5. Unreal Insights (trace GPU+CPU 30 с прохода по маршруту vestibule → kitchen → living → primary → WIC) — эталонный маршрут сохраняется как секвенс `LS_PerfRoute` и прогоняется на каждом Gate 4.
6. Визуализации: Lumen Overview, Nanite Overdraw, Shader Complexity, Light Complexity — кнопки в WBP\_DebugMenu.

Правило: оптимизируем по измерению на эталонном маршруте, а не по ощущению; каждый Gate 4 фиксирует таблицу «профиль → avg/1 % low fps» в проектном wiki.

## 14. Builds, Pixel Streaming, web / mobile / VR, future building

### macOS и Windows build

|  | Windows | macOS |
| --- | --- | --- |
| Сборка | Shipping, DX12 SM6, Pak + IoStore, сжатие Oodle | Shipping, Metal SM6, подпись Developer ID + notarization (без этого Gatekeeper не запустит) |
| Размер | ≈ 8–12 GB на квартиру (текстуры 6 GB + Nanite 2 GB + остальное) | то же |
| Графика | Всё, включая HWRT Lumen, DLSS (плагин), 8K MRQ | Без HWRT, DLSS; та же сцена, профиль MEDIUM/HIGH |
| Доставка | Инсталлер (Inno Setup) или zip; ссылка через S3/Drive | .dmg |
| Кто собирает | Windows build machine (можно удалённо / CI) | Mac разработчика |
| PSO/shader cache | Bundled PSO cache собран по проходу LS\_PerfRoute | то же, Metal compile на первом запуске дольше (loading screen это покрывает) |

Один проект, два таргета, никаких веток под платформу: все отличия — в Device Profiles и scalability.

### Pixel Streaming

Архитектура: сервер с GPU (production — Windows RTX; Mac-хост технически поддержан через VideoToolbox, но без HWRT) запускает shipping-билд с `-PixelStreamingURL`, рядом Signalling Server (Node) и TURN/STUN (coturn); клиент открывает ссылку, получает WebRTC-видео и шлёт ввод. Один экземпляр UE = один клиент (для презентации так и нужно — каждый ходит сам); матчмейкер поднимает экземпляр по запросу.

| Аспект | Оценка |
| --- | --- |
| Hosting | Вариант А — свой PC с RTX 4080 в офисе + статичный IP/Cloudflare Tunnel (0 $/мес, 1 клиент за раз); вариант Б — облако с GPU (AWS g5.xlarge A10G, Azure NV-series, или специализированные: Arcware, Eagle 3D Streaming, Vagon Streams) |
| GPU server | A10G/L4 тянет профиль HIGH в 1080p60 или 1440p30; RTX 4080 — 1440p60 |
| Latency | В пределах штата/региона 40–70 мс — для прогулки незаметно; с другого континента 150+ мс — камера «плывёт»; регион сервера = регион клиента (us-east-1 для Флориды) |
| Cost (облако) | ≈ 1–1.5 $/час работы экземпляра на спот/on-demand GPU + трафик; сервер включается по запросу, гасится через 10 мин простоя → при 10 сессиях по 30 мин в неделю ≈ 30–50 $/мес; платформы-посредники — подписка от ≈ 100–300 $/мес с лимитом часов (цены проверить перед закупкой) |
| Quality | H.264/AV1 1080p–1440p, 10–20 Mbps; тонкие фаски и текстура камня теряют часть детали в кодеке — для «посмотреть квартиру» отлично, для утверждения камня — стиллы MRQ |
| Mobile | Работает в Safari/Chrome на iPhone/iPad без установки; нужен touch-слой UI (виртуальный стик + тап = Interact, drag = look) — встроенный фронтенд Pixel Streaming это умеет, кастомизируем под стиль NOAR |

### Сравнение способов доставки

|  | Native app | Pixel Streaming | Video (MRQ) | Cloud presentation (созвон + шаринг экрана) |
| --- | --- | --- | --- | --- |
| Качество | Максимум | Высокое, минус кодек | Максимум | Среднее (Zoom-сжатие) |
| Интерактив | Да | Да | Нет | Только у дизайнера |
| Порог для клиента | Скачать 10 GB, нужен GPU | Ссылка | Ссылка | Ссылка на встречу |
| Стоимость студии | 0 | Сервер | 0 | 0 |
| Роль | Офис студии, шоурум, клиенты с игровым PC | Главный канал для клиента дома и с iPad | Маркетинг, сайт, reels | Первая встреча |

Рекомендация для студии: Native app в офисе (RTX PC + геймпад) для презентаций; Pixel Streaming как ссылка «посмотрите дома с семьёй» после встречи — стартовать с офисного PC как сервера, облако — когда появятся параллельные клиенты; видео — для сайта и соцсетей.

### Mobile native

Full UE-опыт на iPad/iPhone невозможен без потери сути: Lumen и Nanite на iOS не поддерживаются в нужном качестве, пришлось бы запекать свет и отказаться от конфигуратора. Ответ для мобильных — Pixel Streaming.

### VR-готовность (не реализуется в v1.0)

Что закладываем сейчас, чтобы VR не потребовал remake: (1) вся логика взаимодействия в BPC\_Interaction и интерфейсе, а не в персонаже — VR Pawn с контроллерами переиспользует компонент (trace из руки вместо курсора); (2) UI строится на Common UI / виджетах без привязки к экранным координатам — те же виджеты на Widget Component в мире; (3) никаких screen-space трюков в материалах (только PP\_Highlight, который в VR заменяется на overlay); (4) физический масштаб 1:1 и pivot-ы механизмов — в VR ящик можно тянуть рукой по той же оси (Alpha от положения руки вместо Timeline); (5) скалируемость до Forward Shading + запечённый свет для Quest standalone — поэтому светильники стоят как акторы с правильными физическими значениями, их можно переключить в Static и запечь. PC VR (Quest Link / Vision Pro через streaming) получит тот же Lumen-уровень с профилем MEDIUM.

### Multiple apartments / building

Для одной квартиры World Partition не нужен — обычный persistent level `L_<APT>_Main` с sublevels (Architecture, Lighting, Furniture, Interactive, Decor, Cinematics) через Level Streaming (все Always Loaded; деление нужно для параллельной работы и реимпорта, а не для стриминга). Масштабирование:

| Масштаб | Решение |
| --- | --- |
| 2–20 квартир, каждая отдельно | Один проект, папка на квартиру, свой `L_<APT>_Main`; `BP_ApartmentManager` открывает уровень из DA\_Apartment (`OpenLevel` с loading screen). Билд может содержать одну или все (отдельные .pak на квартиру через chunk assignment — клиент качает только свою) |
| Этаж / здание с переходом между квартирами | World Partition + Data Layers: общие коридоры и оболочка здания — базовый слой; каждая квартира — Level Instance в своём Data Layer, грузится по входу; HLOD для вида с балкона на соседние корпуса |
| Типовые планировки (одна модель × N этажей) | Level Instance одной квартиры, размещённый N раз с разным DA\_Apartment (финиши, вид из окна по этажу) |

**Apartment selector** (будущее): Building → Floor → Apartment → Enter — это три списка из DA\_Building (список DA\_Floor → список DA\_Apartment). Структура DA\_Apartment уже содержит всё нужное; добавляются только два родительских ассета и экран выбора. Ничего в квартирном уровне менять не придётся.

## 15. Master Blueprint system, Blueprint vs C++, plugins, source control, backups

### Связи между системами

Два слоя: **менеджеры** (один экземпляр на уровень, находятся через GameMode) и **акторы квартиры** (сотни, общаются только через интерфейсы, теги и dispatchers). Актор никогда не ссылается на другой актор по имени класса кроме менеджеров.

&#91;embedded content: Master Blueprint system · 4 менеджера, персонаж, интерактивные акторы\]

Стрелка снизу от Interactive actors к LightingManager — это BP\_LightSwitch, вызывающий ToggleGroup; все остальные акторы менеджеров не знают.

| Класс | Роль | Знает о |
| --- | --- | --- |
| BP\_NOAR\_GameInstance | Живёт между уровнями: выбранная квартира, настройки, загрузка/запись SaveGame, язык | SG\_NOAR\_ClientConfiguration, DA\_Apartment |
| BP\_NOAR\_GameMode | Создаёт менеджеры на BeginPlay, спавнит персонажа в стартовой точке DA\_Apartment, держит ссылки на менеджеры (getter для всех) | Все менеджеры |
| BP\_NOAR\_PlayerController | Input contexts (Walk/UI/Photo), курсор, открытие/закрытие виджетов, режим камеры, photo mode | HUD, Character |
| BP\_NOAR\_Character | Движение, две камеры, BPC\_Interaction | BPC\_Interaction |
| BPC\_Interaction | Trace, focus, distance, occlusion, вызов Interact | BPI\_Interactable (только интерфейс) |
| BP\_ApartmentManager | Загрузка DA\_Apartment, реестр комнат, телепорт, предзагрузка ассетов, в будущем — смена квартиры | DA\_Apartment, DA\_Room |
| BP\_MaterialConfigurator | Реестр Configurable-поверхностей, ApplyOption, Reset, текущая конфигурация | DT\_FinishOptions, DA\_MaterialOption, BPC\_ConfigurableSurface |
| BP\_LightingManager | Реестр BP\_InteractiveLight по тегам, ToggleGroup, состояния групп, dispatcher OnGroupChanged | Gameplay Tags |
| BP\_DayNightManager | Пресеты DA\_LightPreset, бленд солнца/неба/HDRI/PP, вызов LightingManager | DA\_LightPreset, LightingManager |
| BP\_InteractiveBase → Door / Drawer / Flap / SlidingDoor / Pocket / LightSwitch / Appliance | Состояния, Timeline, звук, зависимости | Другие interactables только через BPI + soft refs; LightingManager для выключателей |
| BP\_InteractiveLight | Меш + Rect/Spot + emissive, ON/OFF по тегу | LightingManager (подписка) |
| WBP\_\* | Отображение и ввод | Менеджеры через GameMode getters; никогда акторы напрямую |

### Blueprint vs C++

| Система | Blueprint | C++ | Рекомендация |
| --- | --- | --- | --- |
| Interactables, Timeline-анимация | Достаточно, артисты правят сами | Не нужен | **Blueprint** |
| BPC\_Interaction (trace раз в 0.05 с) | Достаточно | Выигрыш < 0.1 мс | **Blueprint** |
| Character / Camera | Достаточно | — | **Blueprint** |
| Material configurator | Достаточно (300 компонентов — мс) | — | **Blueprint** |
| Lighting / DayNight | Достаточно | — | **Blueprint** |
| SaveGame | Достаточно | — | **Blueprint** |
| UI | UMG + Blueprint | — | **Blueprint** |
| Gameplay Tags, structs, enums | — | Определение в C++ даёт стабильные структуры (Blueprint-structs ломают ссылки при редактировании) | **C++ (маленький модуль NOARCore)** |
| Debug menu исключение из shipping | Через DirectoriesToNeverCook — хрупко | `#if !UE_BUILD_SHIPPING` | **C++ в NOARCore** |
| Импорт спецификации (CSV → DA\_Product) | Editor Utility Blueprint | — | **Blueprint (EUW)** |
| Pixel Streaming сигналинг | Плагин, конфиг | — | **Плагин** |
| Будущее: VR grab-кинематика, онлайн-каталог через HTTP | Возможно | Чище | Решить при необходимости |

Итог: проект Blueprint-first с одним крошечным C++-модулем (теги, структуры, сотня строк). Для него нужен Xcode на Mac / VS на Windows, но компилируется он раз в месяц.

### Plugins

| Плагин | Зачем | Статус |
| --- | --- | --- |
| Datasmith Importer (+ Datasmith Content) | Импорт архитектуры | ON |
| Enhanced Input | Ввод | ON (по умолчанию) |
| Movie Render Queue + Movie Render Pipeline | Видео и стиллы | ON |
| Modeling Tools Editor Mode | Правка pivot, фаски, UV прямо в UE | ON (editor only) |
| Control Rig | Leg IK персонажа | ON |
| Gameplay Tags Editor / Gameplay Abilities | Теги (Abilities не нужны) | Tags ON, GAS OFF |
| Common UI | Виджеты с геймпад-навигацией и VR-готовностью | ON |
| MetaSound | Звук | ON (по умолчанию) |
| Pixel Streaming | Стриминг | ON только в Windows-билде |
| NVIDIA DLSS | Upscale на RTX | ON, Windows |
| MetaHuman | Персонаж client build | ON с Phase 4b |
| Niagara | Пока не используется (пыль в луче — кандидат v1.1) | ON (по умолчанию), ассетов нет |
| Water, Chaos Cloth, Landscape, Paper2D, VR plugins | Не нужны | OFF |

### Source control

**Perforce (Helix Core)** — для UE это стандарт: бинарные .uasset с эксклюзивным checkout (два человека не могут сломать один блупринт), бесплатно до 5 пользователей / 20 workspaces, сервер — офисный Windows PC или дешёвая VPS с 1 TB. Git LFS работает, но без локов конфликт в .umap или BP — это потеря работы одного из двоих; для команды из 1 человека Git LFS допустим, от 2 — Perforce. В обоих случаях: `Saved/`, `Intermediate/`, `DerivedDataCache/`, `Binaries/` не версионируются; исходные FBX/текстуры лежат в отдельном depot `//NOAR_Source/` (не в Content). Shared DDC на сетевом диске экономит часы компиляции шейдеров на Mac.

### Backups (3-2-1)

| Уровень | Что | Как часто | Где |
| --- | --- | --- | --- |
| 1 | Perforce depot (checkpoint + journal) | Ежедневно автоматом | Второй диск сервера |
| 2 | Тот же checkpoint + depot files | Ежедневно | Облако (Backblaze B2 / S3 Glacier), versioning ON, 90 дней |
| 3 | Архив проекта на каждом Gate + каждый клиентский билд | По событию | Оффлайн-диск в офисе + облако |
| 4 | Исходники (CAD, сканы слэбов, HDRI, PSD) | При добавлении | //NOAR\_Source + облако |
| Проверка | Восстановление проекта из облачного бэкапа на чистую машину | Раз в квартал | — |

Локально на каждой машине — встроенный UE autosave/backup (`Saved/Backup`, 10 копий) и Time Machine/File History; это защита от краша, не от потери.

## 16. Roadmap: MVP, фазы, quality gates, пошаговый план

Сначала одна комната доказывает технологию, потом масштаб. **PROTOTYPE ROOM = кухня + столовая Dempster** (самый плотный millwork: существующая белая кухня, витрина с бронзовым стеклом, остров с Neolith, панели M01, стена с Cipollino и бронзовым зеркалом, окна на балкон) — если это выглядит и работает, остальная квартира — только объём.

### MVP (конец Phase 8)

- Кухня + столовая с окнами и балконным бэкдропом; остальные проёмы закрыты.
- Day / Evening.
- 6 фасадов (3 hinged L/R, 1 AVENTOS HK, 2 стеклянных витрины), 4 ящика LEGRABOX (один с внутренним лотком), холодильник, посудомойка.
- 2 выключателя: undercabinet LED, витрина; LED в шкафу при открытии.
- Персонаж Manny, 3P ↔ 1P.
- Конфигуратор: cabinet finish (3 опции), countertop (3), wall panel (2).
- Photo mode, SaveGame, splash с логотипом.
- Билд macOS + Windows, 60 fps на RTX 4070 1440p HIGH, 30 fps на M2 Max MEDIUM.

### Фазы и quality gates

| Фаза | Содержание | Оценка (1 человек) | Gate |
| --- | --- | --- | --- |
| 1 Empty room prototype | Проект, settings, папки, коробка кухни-столовой из Datasmith, окна, бэкдроп | 3 дня | — |
| 2 Lighting | Солнце/небо, Lumen, VSM, exposure, PP\_NOAR\_MASTER, 10 светильников | 4 дня | **GATE 1 VISUAL QUALITY** (с временными материалами) |
| 3 Materials | 7 мастеров, hero-текстуры камня и дуба, стекло, зеркало, металл | 6 дней | **GATE 1 повторно** — скриншот рядом с референсом и рендером дизайнера |
| 4 Character + camera | BP\_NOAR\_Character, spring arm, 1P, ABP, zones; решение Manny vs MetaHuman vs custom avatar с замером GPU и теней | 4 дня | **GATE 2 CAMERA FEEL** — 5 минут прогулки без раздражения, камера ни разу в стене |
| 5 Door interaction | BPI, BPC\_Interaction, BP\_InteractiveBase, Door, highlight, prompt, звук | 5 дней | — |
| 6 Drawer + mechanisms | Drawer, Flap (HK), зависимости, occlusion, appliances, EUW\_MillworkAssembler | 5 дней | **GATE 3 INTERACTION** — новый человек открывает дверь за 10 с без подсказки |
| 7 Lighting interaction | BP\_InteractiveLight, LightSwitch, LightingManager, DayNight | 3 дня | — |
| 8 Material configurator | DA/DT, BP\_MaterialConfigurator, WBP\_ConfigPanel, SaveGame, Reset | 5 дней | **GATE 4 PERFORMANCE** на прототипе (LS\_PerfRoute) |
| 9 Full apartment | Все комнаты, \~100 фасадов/ящиков, мебель, декор, все светильники, телепорт | 15–20 дней | GATE 1–3 на всей квартире |
| 10 Optimization | Бюджеты, scalability, PSO cache, Mac-профиль | 5 дней | **GATE 4 PERFORMANCE** финал |
| 11 Client UI | Splash, HUD, Rooms/floor plan, Settings, Photo mode, локализация, аудио, финальный аватар (выбран на Gate 2) | 6 дней | **GATE 5 CLIENT EXPERIENCE** — тест с двумя людьми не из команды |
| 12 Final build | Shipping Win/Mac, подпись, инсталлер, MRQ-стиллы и hero-видео, Pixel Streaming тест | 4 дня | Сдача |

Критерии gates фиксируются до начала фазы, принимает дизайнер студии (не разработчик). Проект не переходит к Phase 9, пока Gate 1–4 не приняты на прототипе — это защита от сценария «200 шкафов на неправильной архитектуре».

&#91;embedded content: Roadmap · 12 фаз, 5 gates, две линии\]

Ромбы — gates на границе фаз; залитый ромб — точка, после которой линия считается принятой.

К GATE 1 на каждом milestone добавляется Ground Truth Render (Path Tracer того же hero-кадра, раздел 6). Gate 4 на прототипе прогоняется на трёх профилях железа плюс Lumen Lite на Mac — именно там измеряется реальный выигрыш Medium-уровней.

### Пошаговый план

1. **Create project**: UE 5.5+, Third Person / Blueprint / Max quality / no starter content; переименовать в `NOAR`; создать папки раздела 2; удалить шаблонный уровень; подключить Perforce; первый submit.
2. **Configure rendering**: таблица раздела 1 в DefaultEngine.ini, device profiles Mac/Windows, DefaultScalability.ini с NOAR\_LOW…ULTRA, плагины раздела 15; создать модуль NOARCore с Gameplay Tags.
3. **Import prototype room**: Datasmith кухни-столовой в Sublevel\_Architecture, collision shell, окна с тонированным стеклом, HDRI backdrop; статичный millwork как FBX.
4. **Set Lumen**: солнце/Sky Atmosphere/Sky Light, PP\_NOAR\_MASTER, CVars раздела 6, exposure; 10 светильников по электрическому плану; сравнить с референсом → GATE 1.
5. **Create master materials**: 7 мастеров + MF\_RoughnessBreakup, MF\_TileBreaker; MI для M01, LAM01, LAM02, SS01, SS02, GL01, GL02, PT01, HW01; hero-текстуры слэбов (запросить фото слэбов у Ciot/Marbella); GATE 1 повторно.
6. **Create character**: BP\_NOAR\_Character с параметрами раздела 7, IMC\_NOAR\_Walk, ABP с BS\_1D и turn-in-place.
7. **Create camera**: spring arm, lag, collision recovery, BPV\_CameraZone, 1P камера и бленд, fade персонажа → GATE 2.
8. **Create interaction interface**: BPI\_Interactable, BPC\_Interaction (trace, distance, occlusion, dependency), PP\_Highlight, WBP\_InteractionPrompt, канал коллизии Interactable.
9. **Create door blueprint**: BP\_InteractiveBase + soft-close curve + DA\_SoundSet; BP\_InteractiveDoor L/R; первые 3 фасада с правильными pivot из CAD.
10. **Create drawer blueprint**: BP\_InteractiveDrawer + TRAY с зависимостью; BP\_InteractiveFlap (HK); appliances; EUW\_MillworkAssembler → GATE 3.
11. **Create lighting system**: BP\_InteractiveLight (LED витрины, undercab), BP\_LightSwitch, BP\_LightingManager, BP\_DayNightManager с Day/Evening.
12. **Create configurator**: структуры DA/DT, BPC\_ConfigurableSurface на фасадах/столешнице/панелях, BP\_MaterialConfigurator, WBP\_ConfigPanel, GameInstance + SaveGame, Reset.
13. **Optimize prototype**: LS\_PerfRoute, stat unit/ProfileGPU, бюджет раздела 13 на RTX и Mac, PSO cache → GATE 4.
14. **Approve quality**: демо прототипа дизайнеру и одному клиенту; список правок → фикс → тег `v0.5-prototype` в Perforce, архив.
15. **Scale to full apartment**: Datasmith всей квартиры; millwork по комнатам (WIC → primary → ensuite → guest → den → powder → laundry → foyer → living); мебель и декор; все светильники L01–L15; DA\_Room × 10; зеркала с planar reflection.
16. **Full-apartment gates**: GATE 1–3 по комнатам, список дефектов в трекере.
17. **Optimization pass**: бюджеты на всей квартире, scalability-тест на трёх профилях, текстурный пул → GATE 4 финал.
18. **Client UI**: WBP\_Splash, HUD, Rooms + floor plan, Settings, Photo mode, Product cards, локализация EN/RU, аудио, финальный аватар → GATE 5.
19. **Cinematics**: LS\_DEM\_Hero\_30s, 2 reels, 12 стиллов 4K/6K через MRQ.
20. **Final client build**: Shipping Windows (инсталлер) + macOS (notarized .dmg), PSO cache, тест на чистых машинах, Pixel Streaming с офисного PC, архив `v1.0`, бэкап.

Суммарно для одного technical artist — около 14–16 рабочих недель до v1.0, из них прототип — первые 7. Параллельный 3D-артист на millwork и мебель сокращает Phase 9 вдвое.
