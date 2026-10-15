# -*- coding: utf-8 -*-
# GenryTB V1: a self-running check of the hub's screens in the game (copied to game/mods/_genry_tb_preview by
# hand for a check, removed by tools/selfrun.ps1). Shows each thing for a few seconds, then quits.

init 999 python:
    config.label_overrides['splashscreen'] = '_genry_tb_autostart'
    config.label_overrides['main_menu'] = '_genry_tb_autostart'
    config.modal_blocks_pause = False

label _genry_tb_autostart:
    python:
        config.label_overrides.pop('main_menu', None)
        config.label_overrides.pop('splashscreen', None)
        _preferences.fullscreen = False
    stop music
    scene bg road_night
    show Fox Normal m_night 00 good ahead at genry_center, genry_mv_still, genry_mv_none
    # a list of choices; the same against the clock
    show screen genry_tb_choice([(u"Бежать", "_genry_tb_done"), (u"Стоять на месте", "_genry_tb_done"), (u"Промолчать", "_genry_tb_done")], 0)
    $ renpy.pause(4.0, hard=True)
    hide screen genry_tb_choice
    show screen genry_tb_choice([(u"Бежать", "_genry_tb_done"), (u"Стоять на месте", "_genry_tb_done"), (u"Промолчать", "_genry_tb_done")], 30)
    $ renpy.pause(4.0, hard=True)
    hide screen genry_tb_choice
    # a notice
    show screen genry_tb_poster(u"ПРОПАЛА ДЕВОЧКА", u"Ушла из дома 12 декабря и не вернулась. Была в лисьей маске. Видевших просим сообщить в милицию.", "Fox Normal m_night 00 good ahead", (805, 124, 300, 300))
    $ renpy.pause(4.0, hard=True)
    hide screen genry_tb_poster
    # a memory with snow; the picture choice
    show genry_tb_snow zorder 80 at genry_tb_weather
    show screen genry_tb_film("memory")
    show screen genry_tb_spots([(u"ВЗЯТЬ", "_genry_tb_done", 960, 760), (u"ОТКАЗАТЬСЯ", "_genry_tb_done", 1500, 240)])
    $ renpy.pause(4.0, hard=True)
label _genry_tb_done:
    $ renpy.quit()
