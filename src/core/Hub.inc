R"HUB(# -*- coding: utf-8 -*-
# GenryTB V1 hub @VERSION@: the mods made with GenryTB, «Моды» in Tiny Bunny's main menu.
# Written by GenryTB V1 - edits here are overwritten.

init -5 python:
    genry_tb_mods = []

    def genry_tb_register(mod_id, name, author, start, desc=u"", scenes=0, choices=0, ach=0, adult=False):
        for m in list(genry_tb_mods):
            if m["id"] == mod_id:
                genry_tb_mods.remove(m)
        cover = "mods/%s/cover.jpg" % mod_id
        genry_tb_mods.append(dict(id=mod_id, name=name, author=author, start=start, desc=desc, scenes=scenes,
                                  choices=choices, ach=ach, adult=adult, cover=cover if renpy.loadable(cover) else None))

    # a grown-ups' mod starts only after «тебе есть 18?» (asked once)
    def genry_tb_play(m):
        if m.get("adult") and not persistent.genry_tb_adult_ok:
            return Show("genry_tb_adult_gate", None, m["start"])
        return Start(m["start"])
        genry_tb_mods.sort(key=lambda m: m["name"].lower())

    def genry_tb_count(n, one, few, many):
        # 1 сцена, 2 сцены, 5 сцен
        k = n % 100
        if 11 <= k <= 14:
            w = many
        elif n % 10 == 1:
            w = one
        elif 2 <= n % 10 <= 4:
            w = few
        else:
            w = many
        return u"%d %s" % (n, genry_tb_t(w))

    def genry_tb_stats(m):
        parts = []
        if m["scenes"]:
            parts.append(genry_tb_count(m["scenes"], u"сцена", u"сцены", u"сцен"))
        if m["choices"]:
            parts.append(genry_tb_count(m["choices"], u"выбор", u"выбора", u"выборов"))
        if m.get("ach"):
            got = len([k for k in (persistent.genry_tb_ach or ()) if k.startswith(m["id"] + ":")])
            parts.append(u"{font=DejaVuSans.ttf}★{/font} %d/%d" % (min(got, m["ach"]), m["ach"]))
        return u"  ·  ".join(parts)

    # story mods: a mod's scene plays right before a label of the game (in its own context: the game's
    # screen comes back as it was), or plays instead of it (the story turns there)
    genry_tb_inserts = {}

    def genry_tb_hook_insert(label, scene):
        lst = genry_tb_inserts.setdefault(label, [])
        if scene not in lst:
            lst.append(scene)

    def genry_tb_hook_replace(label, scene):
        config.label_overrides[label] = scene

    def genry_tb_label_hook(name, abnormal):
        scenes = genry_tb_inserts.get(name)
        if not scenes or getattr(store, "_genry_tb_inside", False):
            return
        store._genry_tb_inside = True
        try:
            for scene in scenes:
                if renpy.has_label(scene):
                    renpy.call_in_new_context(scene)
        finally:
            store._genry_tb_inside = False

    if "_genry_tb_prev_label_callback" not in globals():
        _genry_tb_prev_label_callback = config.label_callback

        def _genry_tb_label_callback(name, abnormal):
            if _genry_tb_prev_label_callback is not None:
                _genry_tb_prev_label_callback(name, abnormal)
            genry_tb_label_hook(name, abnormal)

        config.label_callback = _genry_tb_label_callback

    if 'genry_tb_mods_entry' not in config.always_shown_screens:
        config.always_shown_screens.append('genry_tb_mods_entry')

# where a mod stands a sprite (показать ... слева / справа / центр / далеко / близко)
# shown anew - it is there at once; already on the screen - it walks over (steps closer, steps back)
transform genry_left:
    xalign 0.5 yalign 1.0
    on show:
        xoffset -480 zoom 1.0
    on replace:
        ease 0.45 xoffset -480 zoom 1.0
transform genry_right:
    xalign 0.5 yalign 1.0
    on show:
        xoffset 480 zoom 1.0
    on replace:
        ease 0.45 xoffset 480 zoom 1.0
transform genry_center:
    xalign 0.5 yalign 1.0
    on show:
        xoffset 0 zoom 1.0
    on replace:
        ease 0.45 xoffset 0 zoom 1.0
transform genry_far:
    xalign 0.5 yalign 1.0
    on show:
        xoffset 0 zoom 0.8
    on replace:
        ease 0.45 xoffset 0 zoom 0.8
transform genry_close:
    xalign 0.5 yalign 1.0
    on show:
        xoffset 0 zoom 1.25
    on replace:
        ease 0.45 xoffset 0 zoom 1.25

# the same places with a size of the sprite's own (показать ... рост 90)
transform genry_at(dx=0, z=1.0):
    xalign 0.5 yalign 1.0
    on show:
        xoffset dx zoom z
    on replace:
        ease 0.45 xoffset dx zoom z

transform genry_tb_fade:
    alpha 0.0
    linear 0.35 alpha 1.0
    on hide:
        linear 0.25 alpha 0.0

transform genry_tb_photo:
    rotate_pad False
    rotate -1.6
    alpha 0.0 yoffset 18
    easeout 0.35 alpha 1.0 yoffset 0

# a button of the game's own kind: its white word, and under the pointer its brush stroke with the word
# in black (the game's own buttons are made of two - a word and a stroke over it; one button here)
style genry_tb_button is default:
    xminimum 250
    yminimum 75
    background None
    hover_background "interface/main_meny/plaska.png"
    hover_sound "sounds/menu/menu-button-select-1.ogg"
    activate_sound "sounds/menu/menu-button-click-1.ogg"
style genry_tb_button_text is default:
    font "font/razor_k.ttf"
    size 40
    color "#ffffff"
    hover_color "#000000"
    xalign 0.5
    yalign 0.5

# «Моды» right under the game's own column of buttons, only on its main menu
screen genry_tb_mods_entry():
    zorder 50
    if genry_tb_mods and renpy.get_screen('main_menu') and not renpy.get_screen('genry_tb_mods') \
            and not renpy.get_screen('load') and not renpy.get_screen('preferences') \
            and not renpy.get_screen('about_me') and not renpy.get_screen('gallery') \
            and not renpy.get_screen('black_screen'):
        # made exactly as the game makes its own: the white word, and over it a button of the brush stroke
        # with the word in black (style main_menu_text) that comes up under the pointer (mm_but)
        fixed:
            xalign 0.47
            ypos (702 if has_endings() else 657)
            xsize 250 ysize 75
            at mm_elements
            textbutton genry_tb_t(u"Моды"):
                style "main_menu_button"
                text_style "main_menu_button_text"
                keyboard_focus False
                action ShowMenu('genry_tb_mods')
            button:
                style "main_menu_button"
                background "interface/main_meny/plaska.png"
                text genry_tb_t(u"Моды") style "main_menu_text"
                at mm_but
                action ShowMenu('genry_tb_mods')

screen genry_tb_mods():
    tag menu
    modal True
    zorder 60
    default sel = 0
    on "show" action Play("test_five", "sounds/menu/menu-window-4.ogg")
    $ cur = genry_tb_mods[sel] if sel < len(genry_tb_mods) else None

    # the main menu's own night: its fog drifting, its snow
    add gui.main_menu_background
    add "menu002_1"
    add "menu002_2"
    add "chastichka_2"
    add "menu001_1"
    add "menu001_2"
    add "chastichka_1_1"
    add Solid("#000000b4")

    fixed:
        at genry_tb_fade

        text genry_tb_t(u"Моды") font "font/razor_k.ttf" size 120 color "#ffffff" xpos 140 ypos 50
        add "interface/polosa.png" xpos 130 ypos 196
        text genry_tb_count(len(genry_tb_mods), u"мод", u"мода", u"модов") font "font/russia.ttf" size 34 color "#9a9a9a" xpos 150 ypos 236

        viewport id "genry_tb_list":
            xpos 130 ypos 300 xsize 790 ysize 600
            mousewheel True
            draggable True
            vbox:
                spacing 8
                for i, m in enumerate(genry_tb_mods):
                    button:
                        xsize 770 ysize 104
                        background (Frame("interface/main_meny/plaska.png", 40, 16) if i == sel else None)
                        hover_background Frame("interface/main_meny/plaska.png", 40, 16)
                        hover_sound "sounds/menu/menu-button-select-3.ogg"
                        activate_sound "sounds/menu/menu-button-click-1.ogg"
                        hovered SetScreenVariable("sel", i)
                        action genry_tb_play(m)
                        vbox:
                            xpos 40 yalign 0.5
                            hbox:
                                spacing 16
                                text m["name"] font "font/razor_k.ttf" size 46 color ("#000000" if i == sel else "#ffffff") hover_color "#000000"
                                if m.get("adult"):
                                    text u"18+" font "font/razor_k.ttf" size 34 color "#d63a3a" yalign 0.6
                            if m["author"]:
                                text m["author"] font "font/russia.ttf" size 28 color ("#3a3a3a" if i == sel else "#a8a8a8") hover_color "#3a3a3a"
        if len(genry_tb_mods) > 5:
            vbar value YScrollValue("genry_tb_list"):
                xpos 932 ypos 300 xsize 6 ysize 600
                base_bar Solid("#ffffff22")
                thumb Solid("#ffffffcc")

        if cur:
            # the mod's photograph: its first scene, as the constructor drew it
            fixed:
                xpos 1030 ypos 130 xsize 800 ysize 540
                at genry_tb_photo
                add Solid("#efefe9") xsize 800 ysize 540
                if cur["cover"]:
                    add cur["cover"] xpos 20 ypos 20 zoom 0.7916667
                else:
                    add Solid("#161616") xpos 20 ypos 20 xsize 760 ysize 428
                    text u"?" font "font/razor_k.ttf" size 200 color "#3a3a3a" xpos 360 ypos 110
                text cur["name"] font "font/razor_k.ttf" size 44 color "#141414" xpos 28 ypos 462
            vbox:
                xpos 1040 ypos 700 xsize 780
                spacing 10
                if cur["desc"]:
                    text cur["desc"] font "font/russia.ttf" size 34 color "#e6e6e6" xsize 780
                if cur.get("adult"):
                    text (u"18+  ·  " + genry_tb_t(u"только для взрослых")) font "font/russia.ttf" size 30 color "#d63a3a"
                if cur["author"]:
                    text (genry_tb_t(u"Автор: ") + cur["author"]) font "font/russia.ttf" size 30 color "#a8a8a8"
                if genry_tb_stats(cur):
                    text genry_tb_stats(cur) font "font/russia.ttf" size 30 color "#a8a8a8"
            button:
                xpos 1560 ypos 950 xsize 250 ysize 75
                background "interface/main_meny/plaska.png"
                hover_sound "sounds/menu/menu-button-select-1.ogg"
                activate_sound "sounds/menu/menu-button-click-1.ogg"
                action genry_tb_play(cur)
                text genry_tb_t(u"Играть") font "font/razor_k.ttf" size 40 color "#000000" xalign 0.5 yalign 0.5

        textbutton genry_tb_t(u"Назад"):
            style "genry_tb_button"
            text_style "genry_tb_button_text"
            xpos 130 ypos 950
            action Return()

    key "game_menu" action Return()

# a grown-ups' mod: asked once, in the game's own manner
screen genry_tb_adult_gate(start):
    modal True
    zorder 90
    add Solid("#000000d8")
    vbox:
        xalign 0.5 yalign 0.42
        spacing 26
        text u"18+" font "font/razor_k.ttf" size 160 color "#d63a3a" xalign 0.5
        text genry_tb_t(u"Этот мод — только для взрослых.") font "font/russia.ttf" size 44 color "#ffffff" xalign 0.5
        text genry_tb_t(u"Тебе есть 18 лет?") font "font/russia.ttf" size 44 color "#cfcfcf" xalign 0.5
        null height 20
        hbox:
            xalign 0.5
            spacing 60
            button:
                xysize (360, 75)
                background "interface/main_meny/plaska.png"
                hover_sound "sounds/menu/menu-button-select-1.ogg"
)HUB"
R"HUB(                activate_sound "sounds/menu/menu-button-click-1.ogg"
                action [SetField(persistent, "genry_tb_adult_ok", True), Hide("genry_tb_adult_gate"), Start(start)]
                text genry_tb_t(u"Да, мне есть 18") font "font/razor_k.ttf" size 36 color "#000000" xalign 0.5 yalign 0.5
            textbutton genry_tb_t(u"Нет"):
                style "genry_tb_button"
                text_style "genry_tb_button_text"
                yalign 0.5
                action Hide("genry_tb_adult_gate")
    key "game_menu" action Hide("genry_tb_adult_gate")

# the game's own kind of choice (its «ВЗЯТЬ / ОТКАЗАТЬСЯ»): glowing spots on the picture, each with its
# word; pointed at, a spot lights up. options: [(word, label, x, y)]
transform genry_tb_spot:
    on idle:
        linear 0.2 alpha .01
    on hover:
        linear 0.2 alpha .8

screen genry_tb_spots(options):
    for cap, target, sx, sy in options:
        fixed:
            imagebutton:
                idle "interface/intercative_button.png"
                focus_mask True
                pos (sx, sy)
                anchor (.5, .5)
                hover_sound "sounds/menu/button-click-4.ogg"
                action Jump(target)
                at genry_tb_spot
            text cap:
                pos (sx, sy)
                anchor (.5, .5)
                style "imagemap_text"
                color "#ffffff"
                outlines [ (absolute(1), "#000000", absolute(0), absolute(0)) ]

# a mod's story is over: КОНЕЦ, then the game's main menu
label genry_tb_the_end:
    window hide
    hide screen genry_tb_film
    show layer master
    python:
        for _genry_c in ("music", "music2", "fon", "ambient", "ambient2", "ambient3", "sound"):
            try:
                renpy.music.stop(channel=_genry_c, fadeout=2.0)
            except Exception:
                pass
    scene black with Dissolve(1.5)
    show text (u"{font=font/razor_k.ttf}{size=130}{color=#ffffff}" + genry_tb_t(u"Конец") + u"{/color}{/size}{/font}") at truecenter with Dissolve(1.0)
    $ renpy.pause(2.5)
    hide text with Dissolve(1.0)
    $ renpy.full_restart()
)HUB"
