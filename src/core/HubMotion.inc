R"HUB(
# ---- the pseudo-motion: «Камера», «Движение», «Погода», «Плёнка» of GenryTB ----
# The moves are the game's own way (show layer master: ..., show X at sprite_fear): pieces of ATL.
@MOTIONS@
# snow over the scene, made as the game makes its blizzards: its own snow pictures drifting, the far ones
# slowly, the near ones fast (image blizzard_d4_evening of the game)
image genry_tb_snow:
    contains:
        "effect/snow/snow02.png"
        parallel:
            xpan 0
            linear 120 xpan -360
            repeat
        parallel:
            ypan 0
            linear 24 ypan -360
            repeat
    contains:
        "effect/snow/snow01.png"
        parallel:
            xpan 0
            linear 80 xpan -360
            repeat
        parallel:
            ypan 0
            linear 14 ypan -360
            repeat
image genry_tb_blizzard:
    contains:
        "effect/snow/snow02.png"
        parallel:
            xpan 0
            linear 14 xpan -360
            repeat
        parallel:
            ypan 0
            linear 12 ypan -360
            repeat
    contains:
        "effect/snow/snow01.png"
        parallel:
            xpan 0
            linear 8 xpan -360
            repeat
        parallel:
            ypan 0
            linear 7 ypan -360
            repeat
    contains:
        "effect/snow/snow00.png"
        parallel:
            xpan 0
            linear 5 xpan -360
            repeat
        parallel:
            ypan 0
            linear 4.5 ypan -360
            repeat
transform genry_tb_weather:
    alpha 0.0
    linear 1.2 alpha 1.0

# the 90s tape: the picture as a worn VHS cassette shows it - lines, colour fringes, a crawling tracking
# bar, grain; and plain television snow. (The same maths draws the constructor's preview.)
init python:
    renpy.register_shader("genry_tb.vhs", variables="""
        uniform float u_time;
        uniform sampler2D tex0;
        varying vec2 v_tex_coord;
    """, fragment_300="""
        vec2 genry_uv = v_tex_coord;
        float genry_bar = fract(genry_uv.y + u_time * 0.07);
        float genry_band = smoothstep(0.0, 0.02, genry_bar) * (1.0 - smoothstep(0.02, 0.07, genry_bar));
        genry_uv.x += sin(genry_uv.y * 40.0 + u_time * 6.0) * 0.0012 + sin(genry_uv.y * 3.0 + u_time * 1.3) * 0.0015 + genry_band * 0.012;
        vec4 genry_mid = texture2D(tex0, genry_uv);
        vec3 genry_col = vec3(texture2D(tex0, genry_uv + vec2(0.0025, 0.0)).r, genry_mid.g, texture2D(tex0, genry_uv - vec2(0.0025, 0.0)).b);
        float genry_line = 0.90 + 0.10 * step(0.5, fract(genry_uv.y * 270.0));
        float genry_n = fract(sin(dot(genry_uv + fract(u_time), vec2(12.9898, 78.233))) * 43758.5453);
        genry_col = genry_col * genry_line + (genry_n - 0.5) * 0.09 + genry_band * 0.08;
        gl_FragColor = vec4(genry_col * genry_mid.a, genry_mid.a);
    """)
    renpy.register_shader("genry_tb.noise", variables="""
        uniform float u_time;
        uniform sampler2D tex0;
        varying vec2 v_tex_coord;
    """, fragment_300="""
        vec4 genry_mid = texture2D(tex0, v_tex_coord);
        vec2 genry_cell = floor(v_tex_coord * vec2(480.0, 270.0));
        float genry_n = fract(sin(dot(genry_cell + fract(u_time * 7.0) * 100.0, vec2(12.9898, 78.233))) * 43758.5453);
        float genry_roll = 0.85 + 0.15 * sin(v_tex_coord.y * 9.0 - u_time * 3.0);
        vec3 genry_col = mix(genry_mid.rgb, vec3(genry_n), 0.38) * genry_roll;
        gl_FragColor = vec4(genry_col * genry_mid.a, genry_mid.a);
    """)

transform genry_film_none:
    mesh False
    shader None
    xoffset 0 yoffset 0
transform genry_film_vhs:
    mesh True
    shader "genry_tb.vhs"
    xoffset 0 yoffset 0
transform genry_film_noise:
    mesh True
    shader "genry_tb.noise"
    xoffset 0 yoffset 0
# the old television: the picture behind its glass (the middle of the picture under the middle of the glass)
# the door peephole: the picture bulged a little under the round glass (the mask over it: genry_tb_film)
transform genry_film_peephole:
    xalign 0.5 yalign 0.5
    zoom 1.22
    blur 0.6
transform genry_film_tv:
    mesh True
    shader "genry_tb.vhs"
    xoffset -123 yoffset -33

# what lies over the picture and does not move with the camera: the scratched frame of a memory (the game's
# own, with its old film's scratches); the old television set
image genry_tb_scratch = Animation(Null(0, 0), 1.0, "effect/old_movie/old_movie_001.png", 0.10, Null(0, 0), 0.05,
    "effect/old_movie/old_movie_002.png", 0.10, Null(0, 0), 2.0, "effect/old_movie/old_movie_003.png", 0.10,
    Null(0, 0), 0.05, "effect/old_movie/old_movie_004.png", 0.10, Null(0, 0), 3.0)

screen genry_tb_film(kind):
    zorder -10
    if kind == "memory":
        add "effect/ramka/ramka4.png"
        add "genry_tb_scratch"
    elif kind == "tv":
        add "goosehunt/tvframe.png"
    elif kind == "peephole":
        add "mods/genry_assets/peephole.png"

# ---- things of the 90s ----
transform genry_tb_paper:
    rotate_pad False
    rotate -1.5
    alpha 0.0 yoffset 24
    easeout 0.3 alpha 1.0 yoffset 0

# a note: paper with a few scrawled words, until clicked
screen genry_tb_note(body):
    modal True
    zorder 70
    add Solid("#00000099")
    fixed:
        xpos 510 ypos 198 xsize 900 ysize 640
        at genry_tb_paper
        add Solid("#e8e2d2")
        text body font "font/razor_k.ttf" size 58 color "#1b1b1b" xpos 70 ypos 56 xsize 760 line_spacing 4
    button:
        xfill True
        yfill True
        background None
        action Return()
    key "dismiss" action Return()

# a "missing" notice: a headline, a photo cut from a sprite (box: where its head is), a few lines
screen genry_tb_poster(title, body, photo, box):
    modal True
    zorder 70
    add Solid("#00000099")
    fixed:
        xpos 580 ypos 60 xsize 760 ysize 960
        at genry_tb_paper
        add Solid("#e8e2d2")
        text title font "font/razor_k.ttf" size 84 color "#141414" xalign 0.5 ypos 34 xmaximum 700 textalign 0.5
        add Solid("#141414") xpos 196 ypos 176 xsize 368 ysize 368
        add Solid("#cfc9b8") xpos 204 ypos 184 xsize 352 ysize 352
        if photo:
            add photo crop box xysize (352, 352) xpos 204 ypos 184
        text body font "font/russia.ttf" size 40 color "#1b1b1b" xalign 0.5 ypos 576 xmaximum 640 textalign 0.5
    button:
        xfill True
        yfill True
        background None
        action Return()
    key "dismiss" action Return()

# a list of choices. (The game's own choice screen is small words in a corner - the game itself chooses on
# its pictures.) Made as the game's main menu is: big white words (on a dark strip: readable over any
# picture, low on the screen - not over a face); under the pointer its brush stroke, the word in black. seconds: against the clock - a strip burning down, time out - the last one.
transform genry_tb_burn(t=5.0):
    xzoom 1.0
    linear t xzoom 0.0

screen genry_tb_choice(options, seconds=0):
    vbox:
        xalign 0.5
        yalign 0.72
        spacing 18
        for cap, target in options:
            button:
                xalign 0.5
                xminimum 760
                yminimum 84
                xpadding 60
                background Solid("#00000088")
                hover_background Frame("interface/main_meny/plaska.png", 40, 16)
                hover_sound "sounds/menu/menu-button-select-1.ogg"
                activate_sound "sounds/menu/menu-button-click-1.ogg"
                action Jump(target)
                text cap:
                    font "font/russia.ttf"
                    size 60
                    color "#ffffff"
                    hover_color "#000000"
                    outlines [ (absolute(1), "#000000", absolute(0), absolute(0)) ]
                    hover_outlines [ ]
                    xalign 0.5
                    yalign 0.5
        if seconds:
            null height 6
            add Solid("#ffffff") xsize 600 ysize 6 xalign 0.5 at genry_tb_burn(seconds)
    if seconds:
        timer seconds action Jump(options[-1][1])

# a struggle (the game's «СПАСТИ» for any mod): press - click or space - faster than it slips away
init python:
    class GenryTbStruggle(object):
        def __init__(self, seconds):
            self.value = 0.3
            self.left = float(seconds)

        def push(self):
            self.value = min(1.0, self.value + 0.065)

        def tick(self, dt):
            self.left -= dt
            self.value = max(0.0, self.value - dt * (0.09 + 0.16 * self.value))
            if self.value >= 1.0:
                return True
            if self.left <= 0.0:
                return False
            return None

transform genry_tb_pulse:
    zoom 1.0
    linear 0.12 zoom 1.06
    linear 0.12 zoom 1.0
    repeat

transform genry_tb_jitter:
    linear 0.04 xoffset 3
    linear 0.04 xoffset -3
    repeat

screen genry_tb_struggle(caption, seconds):
    modal True
    zorder 70
    default st = GenryTbStruggle(seconds)
    add Solid("#00000077")
    text caption font "font/razor_k.ttf" size 120 color "#ffffff" xalign 0.5 ypos 230 outlines [ (absolute(3), "#000000", absolute(0), absolute(0)) ] at genry_tb_pulse
    fixed:
        xalign 0.5
        ypos 520
        xsize 1000
        ysize 60
        at genry_tb_jitter
        add Solid("#000000cc")
        add Solid("#e8e2d2") xsize max(1, int(1000 * st.value)) ysize 60
    text genry_tb_t(u"жми пробел или кликай!") font "font/russia.ttf" size 44 color "#d8d8d8" xalign 0.5 ypos 610
    add Solid("#ffffff") xsize max(1, int(600 * st.left / seconds)) ysize 6 xalign 0.5 ypos 700
    button:
        xfill True
        yfill True
        background None
        action Function(st.push)
    key "K_SPACE" action Function(st.push)
    timer 0.04 repeat True action Function(st.tick, 0.04)

# the end titles: lines rolling up over black; a click skips them
transform genry_tb_roll(t):
    ypos 1100 yanchor 0.0
    linear t ypos -20 yanchor 1.0

screen genry_tb_credits(lines, seconds):
    modal True
    zorder 70
    add Solid("#000000")
    vbox:
        xalign 0.5
        spacing 34
        at genry_tb_roll(seconds)
        for l in lines:
            text l font "font/russia.ttf" size 54 color "#e6e6e6" xalign 0.5 textalign 0.5
    button:
        xfill True
        yfill True
        background None
        action Return()
    timer seconds + 0.3 action Return()

# the game's own {chaos} draws each letter in a random font - fonts its developers never shipped. GenryTB puts
# them in (and its own for every script): here it gets the ones that can write the game's current language.
init 20 python:
    genry_tb_chaos_sets = {
        "cyr": ["Pacifico-Regular.ttf", "RobotoSlab-ExtraBold.ttf", "RobotoSlab-Medium.ttf",
                "mods/genry_fonts/RubikMonoOne-Regular.ttf", "mods/genry_fonts/RubikGlitch-Regular.ttf",
                "mods/genry_fonts/RubikWetPaint-Regular.ttf", "mods/genry_fonts/Neucha.ttf", "mods/genry_fonts/KellySlab-Regular.ttf",
                "mods/genry_fonts/PressStart2P-Regular.ttf", "mods/genry_fonts/RussoOne-Regular.ttf", "mods/genry_fonts/Tektur.ttf",
                "mods/genry_fonts/Jura.ttf", "mods/genry_fonts/Exo2.ttf", "mods/genry_fonts/DelaGothicOne-Regular.ttf"],
        "ja": ["mods/genry_fonts/MochiyPopOne-Regular.ttf", "mods/genry_fonts/DelaGothicOne-Regular.ttf",
               "mods/genry_fonts/RampartOne-Regular.ttf", "mods/genry_fonts/ReggaeOne-Regular.ttf", "mods/genry_fonts/HachiMaruPop-Regular.ttf"],
        "zh": ["mods/genry_fonts/ZCOOLKuaiLe-Regular.ttf", "mods/genry_fonts/ZCOOLQingKeHuangYou-Regular.ttf",
               "mods/genry_fonts/MaShanZheng-Regular.ttf"],
        "ko": ["mods/genry_fonts/BlackHanSans-Regular.ttf", "mods/genry_fonts/DoHyeon-Regular.ttf"],
        "ar": ["mods/genry_fonts/Lalezar-Regular.ttf", "mods/genry_fonts/ReemKufi.ttf"],
    }
    genry_tb_chaos_cache = {}

)HUB"
R"HUB(    def genry_tb_chaos_fonts():
        lang = (preferences.language or "").lower()
        if lang.startswith("jap"):
            key = "ja"
        elif lang.startswith("chin") or lang.startswith("zh"):
            key = "zh"
        elif lang.startswith("kor"):
            key = "ko"
        elif lang.startswith("arab"):
            key = "ar"
        elif lang:
            key = "lat"                      # any Latin language: the game's own list and all the Cyrillic ones
        else:
            key = "cyr"                      # the game's own Russian
        if key not in genry_tb_chaos_cache:
            if key == "lat":
                names = list(getattr(store, "_genry_tb_chaos_original", [])) + genry_tb_chaos_sets["cyr"]
            else:
                names = genry_tb_chaos_sets[key]
            have = [n for n in names if renpy.loadable(n)]
            genry_tb_chaos_cache[key] = have or ["font/russia.ttf"]
        return genry_tb_chaos_cache[key]

    if "ChaosText" in globals() and not getattr(ChaosText, "_genry_tb", False):
        _genry_tb_chaos_original = list(ChaosText.font_list)
        _genry_tb_chaos_render = ChaosText.render

        def _genry_tb_chaos_render_lang(self, *args, **kwargs):
            self.font_list = genry_tb_chaos_fonts()
            return _genry_tb_chaos_render(self, *args, **kwargs)

        ChaosText.render = _genry_tb_chaos_render_lang
        ChaosText._genry_tb = True

# a heading: the game's heading letters, huge, in the middle; they come out of the dark with a slight shiver
transform genry_tb_heading:
    alpha 0.0 zoom 1.08
    easeout 0.8 alpha 1.0 zoom 1.0
    block:
        linear 0.06 xoffset 1
        linear 0.06 xoffset -1
        repeat

screen genry_tb_title(t):
    zorder 75
    add Solid("#00000066")
    text t:
        font "font/razor_k.ttf"
        size 150
        color "#f2f2f2"
        xalign 0.5
        yalign 0.45
        textalign 0.5
        xmaximum 1700
        outlines [ (absolute(3), "#000000", absolute(0), absolute(0)) ]
        at genry_tb_heading

# an ending page, as the game's own: its art (or the scene gone dark), a heading, a column of words, the brush
transform genry_tb_page_in:
    alpha 0.0
    easeout 1.0 alpha 1.0

screen genry_tb_epilogue(heading, body, art, caption):
    modal True
    zorder 75
    fixed at genry_tb_page_in:
        if art:
            for a in art:
                add a
            add Solid("#00000033")
        else:
            add Solid("#000000cc")
        vbox:
            xpos 100
            ypos 100
            xsize 960
            spacing 14
            if heading:
                text heading font "font/russia.ttf" size 62 color "#ffffff" outlines [ (absolute(2), "#000000", absolute(0), absolute(0)) ]
            text body font "font/russia.ttf" size 52 color "#ffffff" justify True line_spacing 6 outlines [ (absolute(2), "#000000", absolute(0), absolute(0)) ]
    button:
        pos (1673, 821)
        xysize (438, 86)
        xanchor 1.0
        background "interface/button.png"
        hover_sound "sounds/menu/menu-button-select-3.ogg"
        action Return()
        text caption:
            align (0.5, 0.5)
            size 50
            color "#000"
        at transform:
            alpha 0.0
            pause 1.2
            linear 0.5 alpha 1.0

# ---- rock-paper-scissors, the game's own hands (episode 3): 1 scissors, 2 paper, 3 rock ----
image genry_tb_suefa_shake:
    "interface/suefa/H_01.png"
    0.11
    "interface/suefa/H_02.png"
    0.11
    "interface/suefa/H_03.png"
    0.11
    "interface/suefa/H_04.png"
    0.11
    "interface/suefa/H_05.png"
    0.11
    "interface/suefa/H_06.png"
    0.11
    repeat

init python:
    def genry_tb_suefa_result(me, foe):
        if me == foe:
            return "draw"
        return "win" if (me, foe) in ((3, 1), (1, 2), (2, 3)) else "lose"

    # the shots live on a class, not in the store: they never go into the saves (a game loaded there gets grey)
    class GenryTbShots(object):
        snap = None
        backdrop = None

    def genry_tb_take_snap():
        try:
            GenryTbShots.snap = im.Data(renpy.screenshot_to_bytes((960, 540)), "genry_tb_snap.png")
        except Exception:
            GenryTbShots.snap = None

    def genry_tb_take_backdrop():
        try:
            GenryTbShots.backdrop = im.Data(renpy.screenshot_to_bytes((1920, 1080)), "genry_tb_backdrop.png")
        except Exception:
            GenryTbShots.backdrop = None

    # the opponent is the left hand (R, Romka's in the game), the player the right (A, Anton's)
    def genry_tb_hand(hands, side, n):
        if side == "R" and hands:
            return "mods/genry_assets/hands/%s/R_0%d.png" % (hands, n)
        return "interface/suefa/%s_0%d.png" % (side, n)

    def genry_tb_check(answer, accepted):
        def norm(x):
            x = (x or u"").lower().replace(u"ё", u"е").strip()
            return u"".join(c for c in x if c.isalnum() or c == u" ").strip()
        a = norm(answer)
        return any(a == norm(x) for x in accepted)

    def genry_tb_dict_link(arg):
        renpy.show_screen("genry_tb_dict", arg)
        renpy.restart_interaction()

    config.hyperlink_handlers["genry_dict"] = genry_tb_dict_link
    genry_tb_seen = set()

# the place blurred into fog, grey as the game's field; the hands sliding in; a fist's three shakes; the loser sinks
transform genry_tb_suefa_back:
    blur 26
    matrixcolor SaturationMatrix(0.0)

transform genry_tb_suefa_in(dx):
    xoffset dx
    easein 0.9 xoffset 0

transform genry_tb_suefa_bob(d):
    yoffset 0
    pause d
    block:
        easein 0.17 yoffset -64
        easeout 0.17 yoffset 0
        repeat 3

transform genry_tb_suefa_sink:
    pause 0.6
    easein 0.6 yoffset 140 alpha 0.55

screen genry_tb_suefa(hands="", place=None, soft=True):
    modal True
    zorder 70
    default phase = "pick"
    default me = 3
    default foe = 3
    if place is None:
        add "interface/suefa/fon.png"
    elif soft:
        add place at genry_tb_suefa_back
        add Solid("#00000038")
    else:
        add place
    text genry_tb_t(u"Камень, ножницы, бумага!") font "font/razor_k.ttf" size 90 color "#ffffff" xalign 0.5 ypos 60 outlines [ (absolute(2), "#000000", absolute(0), absolute(0)) ]
    if phase == "pick":
        add genry_tb_hand(hands, "R", 3) at genry_tb_suefa_in(-1100)
        add "interface/suefa/A_03.png" at genry_tb_suefa_in(1100)
        add "interface/suefa/01.png" at genry_tb_page_in
        add "interface/suefa/1.png" at genry_tb_page_in
        add "interface/suefa/2.png" at genry_tb_page_in
        add "interface/suefa/3.png" at genry_tb_page_in
        for n, bx, bw in ((3, 289, 306), (1, 793, 316), (2, 1310, 296)):
            button:
                pos (bx, 737)
                xysize (bw, 296)
                background None
                hover_background Solid("#ffffff22")
                hover_sound "sounds/menu/menu-button-select-1.ogg"
                action [SetScreenVariable("me", n), SetScreenVariable("foe", renpy.random.randint(1, 3)), SetScreenVariable("phase", "shake")]
    elif phase == "shake":
        if hands:
            add genry_tb_hand(hands, "R", 3) at genry_tb_suefa_bob(0.0)
            add "interface/suefa/A_03.png" at genry_tb_suefa_bob(0.05)
        else:
            add "genry_tb_suefa_shake"
        timer 1.3 action SetScreenVariable("phase", "show")
    else:
        $ res = genry_tb_suefa_result(me, foe)
        add genry_tb_hand(hands, "R", foe):
            if res == "win":
                at genry_tb_suefa_sink
        add genry_tb_hand(hands, "A", me):
            if res == "lose":
                at genry_tb_suefa_sink
        text genry_tb_t(u"ПОБЕДА!" if res == "win" else u"ПРОИГРЫШ" if res == "lose" else u"НИЧЬЯ. ЕЩЁ РАЗ!"):
            font "font/razor_k.ttf"
            size 110
            color "#ffffff"
            xalign 0.5
            ypos 820
            outlines [ (absolute(3), "#000000", absolute(0), absolute(0)) ]
            at genry_tb_heading
        timer 1.8 action Return(res)

# ---- the polaroid: raised, a flash, the card of this very shot ----
transform genry_tb_raise:
    yoffset 420 alpha 0.0
    easeout 0.5 yoffset 0 alpha 1.0

transform genry_tb_flash_out:
    alpha 1.0
    linear 0.4 alpha 0.0

transform genry_tb_card:
    rotate_pad False
    rotate -3
    alpha 0.0 yoffset 60
    easeout 0.5 alpha 1.0 yoffset 0

screen genry_tb_polaroid(snap, caption):
    modal True
    zorder 75
    default phase = 0
    if phase == 0:
        add "interface/suefa/Poloroid_hand.png" at genry_tb_raise
        timer 1.2 action [Play("sound", "sounds/menu/menu-button-click-1.ogg"), SetScreenVariable("phase", 1)]
    elif phase == 1:
        add Solid("#ffffff") at genry_tb_flash_out
        timer 0.4 action SetScreenVariable("phase", 2)
    else:
        add Solid("#000000aa")
        fixed at genry_tb_card:
            xpos 520
            ypos 100
            xsize 880
            ysize 880
            add Solid("#f1ede4")
            add Transform(snap or Solid("#6e6e6e"), xysize=(800, 450)) xpos 40 ypos 40
            text caption font "font/razor_k.ttf" size 66 color "#222222" xpos 60 ypos 560 xmaximum 760
        button:
            xfill True
            yfill True
            background None
            action Return()
        key "dismiss" action Return()

# ---- осмотр: spots on the picture; a spot looked at is dimmed; a way out ends it ----
screen genry_tb_explore(items):
    zorder 60
    for cap, sx, sy, note, target in items:
        fixed:
            imagebutton:
                idle "interface/intercative_button.png"
                focus_mask True
                pos (sx, sy)
                anchor (.5, .5)
                hover_sound "sounds/menu/button-click-4.ogg"
                action [Function(genry_tb_seen.add, cap), Return((note, target))]
                at genry_tb_spot
            text cap:
                pos (sx, sy)
                anchor (.5, .5)
                style "imagemap_text"
                color ("#9a9a9a" if cap in genry_tb_seen and not target else "#ffffff")
                outlines [ (absolute(1), "#000000", absolute(0), absolute(0)) ]

# ---- a word's note (словарь), on paper ----
screen genry_tb_dict(t):
    modal True
    zorder 90
    add Solid("#00000066")
    fixed:
        xpos 560
        ypos 330
        xsize 800
        ysize 380
        at genry_tb_paper
        add Solid("#e8e2d2")
        text t font "font/russia.ttf" size 46 color "#1b1b1b" xpos 50 ypos 44 xsize 700 line_spacing 4
    button:
        xfill True
        yfill True
        background None
        action Hide("genry_tb_dict")

# ---- загадка: the question on paper, the answer written under it (Enter - done) ----
transform genry_tb_caret:
    alpha 1.0
    linear 0.5 alpha 0.25
    linear 0.5 alpha 1.0
    repeat

screen genry_tb_riddle(prompt):
    modal True
    zorder 70
    add Solid("#00000099")
    fixed:
        xpos 460 ypos 230 xsize 1000 ysize 560
        at genry_tb_paper
        add Solid("#e8e2d2")
        text prompt font "font/razor_k.ttf" size 60 color "#1b1b1b" xpos 70 ypos 60 xsize 860 line_spacing 4
        add Solid("#1b1b1b55") xpos 70 ypos 400 xsize 860 ysize 3
        input id "input":
            font "font/russia.ttf"
            size 64
            color "#7a1010"
            xpos 80
            ypos 318
            xmaximum 840
            copypaste True
        text genry_tb_t(u"впиши ответ и нажми Enter") font "font/russia.ttf" size 34 color "#1b1b1b99" xpos 70 ypos 440

# ---- пазл: a picture in pieces on the table of the poster game (episode 4), its paper sounds ----
init python:
    class GenryTbPuzzle(object):
        W, H, X0, Y0 = 1200, 675, 360, 170

        def __init__(self, pic, cols, rows):
)HUB"
R"HUB(            self.base = Transform(pic, xysize=(self.W, self.H), fit="cover")
            self.pw, self.ph = self.W // cols, self.H // rows
            self.pieces = []
            for r in range(rows):
                for c in range(cols):
                    img = Transform(self.base, crop=(c * self.pw, r * self.ph, self.pw, self.ph))
                    tx, ty = self.X0 + c * self.pw, self.Y0 + r * self.ph
                    # scattered over the table, never right on its place
                    while True:
                        x = renpy.random.randint(20, 1900 - self.pw)
                        y = renpy.random.randint(20, 1060 - self.ph)
                        if abs(x - tx) > 120 or abs(y - ty) > 120:
                            break
                    self.pieces.append(dict(key=str(len(self.pieces)), img=img, target=(tx, ty), pos=(x, y), done=False))

        def dragged(self, drags, drop):
            d = drags[0]
            p = self.pieces[int(d.drag_name)]
            tx, ty = p["target"]
            if abs(d.x - tx) < 42 and abs(d.y - ty) < 42:
                d.snap(tx, ty, 0.12)
                p["pos"] = (tx, ty)
                p["done"] = True
                d.draggable = False
                d.bottom()
                renpy.play("sounds/poster_game/sfx_paper_drag_%d.ogg" % renpy.random.randint(1, 5), channel="sound")
            else:
                p["pos"] = (d.x, d.y)
            if all(x["done"] for x in self.pieces):
                return True
            renpy.restart_interaction()
            return None

    def genry_tb_puzzle_grab(drags):
        renpy.play("sounds/poster_game/sfx_paper_activ_%d.ogg" % renpy.random.randint(1, 5), channel="sound")

screen genry_tb_puzzle(pz):
    modal True
    zorder 70
    add "minigame/ss/fon.png"
    add Solid("#00000040") pos (pz.X0 - 10, pz.Y0 - 10) xysize (pz.W + 20, pz.H + 20)
    add Transform(pz.base, alpha=0.14) pos (pz.X0, pz.Y0)
    draggroup:
        for p in pz.pieces:
            drag:
                drag_name p["key"]
                draggable (not p["done"])
                droppable False
                activated genry_tb_puzzle_grab
                dragged pz.dragged
                pos p["pos"]
                fixed:
                    fit_first True
                    if not p["done"]:
                        add Solid("#00000066") xysize (pz.pw, pz.ph) offset (8, 9)
                    add p["img"]
    textbutton genry_tb_t(u"Собрать сразу"):
        style "genry_tb_button"
        text_style "genry_tb_button_text"
        xalign 0.98 ypos 1000
        action Return(True)

# ---- предмет: the game's hand (Anton's pinch, the gum insert of episode 3) holding any picture ----
transform genry_tb_hand_up:
    yoffset 60 alpha 0.0
    parallel:
        linear 0.5 alpha 1.0
    parallel:
        easeout 0.5 yoffset 0

screen genry_tb_inhand(pic, caption):
    modal True
    zorder 70
    add Solid("#000000b0")
    fixed at genry_tb_hand_up:
        fixed:
            xysize (480, 360)
            pos (800, 360)
            anchor (0.5, 0.5)
            at transform:
                rotate -4
                rotate_pad False
            add Solid("#ece8df")
            add Solid("#2a2a2a") xysize (452, 332) align (0.5, 0.5)
            add Transform(pic, fit="contain", xysize=(452, 332)) align (0.5, 0.5)
        add "interface/suefa/vkladush_0.png"
    if caption:
        text caption font "font/razor_k.ttf" size 64 color "#ffffff" xalign 0.5 ypos 930 textalign 0.5 outlines [ (absolute(2), "#000000", absolute(0), absolute(0)) ]
    button:
        xfill True
        yfill True
        background None
        action Return()
    key "dismiss" action Return()

# ---- the camera after the mouse (as the game's menu drifts) ----
init python:
    def genry_cam_follow(trans, st, at):
        x, y = renpy.get_mouse_pos()
        tx = -(x - 960.0) / 960.0 * 46.0
        ty = -(y - 540.0) / 540.0 * 26.0
        trans.xoffset = (trans.xoffset or 0) + (tx - (trans.xoffset or 0)) * 0.08
        trans.yoffset = (trans.yoffset or 0) + (ty - (trans.yoffset or 0)) * 0.08
        return 0

# ---- достижение: a trophy slides in at the corner; once each, remembered for good ----
init python:
    def genry_tb_achieve(mod, title, body):
        if persistent.genry_tb_ach is None:
            persistent.genry_tb_ach = set()
        key = mod + ":" + title
        if key in persistent.genry_tb_ach:
            return
        persistent.genry_tb_ach.add(key)
        renpy.save_persistent()
        renpy.play("sounds/menu/menu-button-click-1.ogg", channel="sound")
        renpy.show_screen("genry_tb_ach", title, body)

transform genry_tb_ach_in:
    xoffset 700 alpha 0.0
    easein 0.45 xoffset 0 alpha 1.0
    pause 3.6
    easeout 0.45 xoffset 700 alpha 0.0

screen genry_tb_ach(title, body):
    zorder 95
    timer 4.6 action Hide("genry_tb_ach")
    fixed at genry_tb_ach_in:
        xpos 1260 ypos 40 xsize 620 ysize 150
        add Solid("#0e0e10e6")
        add Solid("#d63a3a") xsize 6
        text u"★" font "DejaVuSans.ttf" size 50 color "#e8e2d2" xpos 70 ypos 75 anchor (0.5, 0.5)
        text genry_tb_t(u"ДОСТИЖЕНИЕ") font "font/russia.ttf" size 30 color "#96969c" xpos 140 ypos 14
        text title font "font/razor_k.ttf" size 56 color "#ffffff" xpos 140 ypos 44 xmaximum 460 outlines [ (absolute(2), "#000000", absolute(0), absolute(0)) ]
        text body font "font/russia.ttf" size 30 color "#cfcfcf" xpos 140 ypos 108 xmaximum 460

# ---- телефон: the game's push-button phone (episode 5), on the layer that is zoomed close ----
init python:
    genry_tb_phone_number = ""

    def genry_tb_phone_add(d):
        store.genry_tb_phone_number = (store.genry_tb_phone_number + str(d))[-10:]

screen genry_tb_phone_display():
    layer "master"
    text "[genry_tb_phone_number]":
        anchor (1., .5)
        pos (1210, 312)
        size 30
        color "#000"
        font "font/JDK Don Digit.ttf"
        outlines [(1, "#666", 0, 0)]

screen genry_tb_phone_keys():
    modal True
    layer "master"
    default reset_hover = False
    for i in range(10):
        imagebutton:
            focus_mask True
            idle "d5_polina_phone_k_" + str(i) + "_idle"
            hover "d5_polina_phone_k_" + str(i) + "_hover"
            activate_sound "sounds/vhs/button_2.ogg"
            action [Function(genry_tb_phone_add, i), Return("digit")]
    if reset_hover:
        add "d5_polina_phone_k_reset_hover"
    else:
        add "d5_polina_phone_k_reset_idle"
    if genry_tb_phone_number:
        button:
            anchor (.5, .5)
            pos (925, 350)
            xysize (200, 200)
            hovered SetScreenVariable("reset_hover", True)
            unhovered SetScreenVariable("reset_hover", False)
            activate_sound "sounds/sfx_phone_disk.ogg"
            action Return("reset")
            text genry_tb_t(u"СБРОС"):
                align (.5, 1.)
                style "imagemap_text"
                color "#ffffff"
                outlines [ (absolute(1), "#000000", absolute(0), absolute(0)) ]
)HUB"
