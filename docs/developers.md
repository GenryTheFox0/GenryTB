# Для разработчиков

[← Вики](README.md)

## Что нужно

- Windows 10 / 11 x64;
- Visual Studio 2022 (или Build Tools) с компонентом «Разработка классических приложений на C++»;
- Qt 6.8 для MSVC 2022 64-bit, модули: Quick, QuickControls2, Multimedia, ShaderTools, Concurrent;
- CMake 3.21+ и Ninja;
- установленный «Зайчик» из Steam: без него не запустится ни программа, ни самотест.

## Собрать

Из «x64 Native Tools Command Prompt for VS 2022»:

```
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=C:/Qt/6.8.3/msvc2022_64
cmake --build build
```

Получится:

| Файл | Что это |
|---|---|
| `build/GenryTB.exe` | сама программа |
| `build/tb_cli.exe` | ядро из командной строки: `check`, `compile`, `install`, `run`, `frame`, `sprite`, `bg` |
| `build/tb_selftest.exe` | самотест: ядро против настоящей игры (должно быть «78 ok, 0 failed») |
| `build/GenryTB_Setup.exe` | загрузчик установщика (без программы внутри) |

Запуск из `build/`: добавь `C:\Qt\6.8.3\msvc2022_64\bin` в `PATH`. Программа сама найдёт `data/`, поднимаясь по
папкам вверх. Для отдельной папки с программой: `windeployqt --qmldir qml <папка>\GenryTB.exe`.

`-DTB_DEV=ON` включает сборку разработчика (экспериментальные штуки, которых нет в релизе). Релиз собирается без него.

## Релиз

```
set GENRYTB_QT=C:\Qt\6.8.3\msvc2022_64
python tools/make_release.py
```

Скрипт собирает Release в `build_release/`, гоняет самотест, урезает Qt до того, что реально нужно, кладёт рядом
Visual C++ runtime, через `dumpbin` доказывает, что ни одна DLL не потеряна, и пакует `dist/github/`:
`GenryTB_Setup.exe` (загрузчик + zip в хвосте) и `GenryTB_portable.zip`.

## Как устроено

```
src/core/      ядро (tb_core), без интерфейса
  Rpa          чтение archive.rpa игры (RPA-3.0) и её папки, поиск игры в Steam
  Catalog      что есть в игре: персонажи, спрайты слоями, фоны, музыка, звуки (data/tb_catalog.json)
  Story        язык мода: story.txt -> шаги; все ошибки с подсказками
  Compiler     шаги -> Ren'Py (.rpy)
  Hub*.inc     хаб в игре: кнопка «Моды», экраны мини-игр, достижения, переводы хаба
  Motion       одна таблица камеры и движений: её играет и превью, и игра
  Renderer     превью: кадр собирается так же, как его собирает игра
  Builder      установка мода в game/mods, шрифты, ассеты
src/app/       Engine (мост к QML), Blocks (карточки <-> строки), I18n
qml/           интерфейс: Launcher (главное меню), Editor, BlockCard, PreviewPane…
shaders/       VHS и помехи
data/          каталог, руки и места суефа, шрифты (OFL / Apache), переводы, флаги, иконка
tools/         самотест, CLI, установщик, релиз, генераторы, переводы
```

**Хаб — сырые строки C++.** MSVC не берёт строковый литерал длиннее ~16 КБ (C2026). Поэтому `Hub*.inc` порезан на
куски по 12 КБ. Поправил `.inc` — прогони `python tools/split_hub.py`.

**Каталог игры** (`data/tb_catalog.json`) собирает `tools/make_tb_catalog.py` из скриптов самой игры. Скриптов игры
в репозитории нет: распакуй их декомпилятором [unrpyc](https://github.com/CensoredUsername/unrpyc) в `tools/tb_rpyc/`,
а сам unrpyc положи в `tools/unrpyc/`. Тот же скрипт делает `data/tb_voice.json` (записанные фразы с их словами для
поиска голоса по тексту). Этого файла тоже нет в репозитории, потому что в нём текст игры. Без него просто не работает
поиск голоса по словам.

**Руки и места суефа** (`data/assets/`) уже готовы. Их делали `tools/gen_hands.py` и `tools/gen_suefa_bg.py` на
ComfyUI с Qwen-Image-Edit 2511 + Lightning. Графы ComfyUI (`graphs.py`, папка в `GENRY_NEURO_LITE`) в репозиторий не
входят.

**Проверки картинкой:** `GenryTB.exe --shot <страница> <out.png> [--project <id>]` рисует страницу программы в файл
(`tools/shot.ps1`, `tools/shotb.ps1`). `tools/selfrun.ps1` запускает игру в своём окне со своей папкой сохранений,
фотографирует его и закрывает.

## Прислать правку

См. [CONTRIBUTING](../CONTRIBUTING.md): ветка, самотест зелёный, pull request с «до/после».
