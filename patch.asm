; Mad Stalker - Full Metal Forth: Window HUD -> sprite HUD
;
; Sparse overlay source in the same ORG-based style as the YuuYuu/Comix Zone
; patches.  build.bat assembles this complete sparse overlay and applies it to
; the verified original ROM through the standard S-record patching workflow.

        include "SRC/ports.inc"
        include "SRC/ram_addrs.inc"
        include "SRC/hud_symbols.inc"

; Keep the Window disabled at gameplay initialization and level teardown.
        org     ORIGIN_GAME_LINK_CALL
        jsr     HUD_LINK_SPRITES

        org     ORIGIN_VBLANK_TAIL
        jmp     HUD_VBLANK_TAIL

; These shared routines are also used by non-game screens that still expose
; the Window.  Limit them to the three visible compatibility rows so they do
; not touch the gameplay sprite atlas.
        org     ORIGIN_WINDOW_CLEAR_A_COUNT
        dc.w    $005F                       ; 96 words = three 32-cell rows

        org     ORIGIN_WINDOW_CLEAR_B_COUNT
        dc.w    $005F

        org     ORIGIN_DISPLAY_CLEAR_COUNT
        dc.w    $005F

; Shared display/system initialization may still clear the top three Window
; rows for non-game screens.  Hidden rows contain the gameplay sprite atlas.
        org     ORIGIN_SYSTEM_CLEAR_COUNT
        dc.w    $005F

; Stage Clear originally exposes all 28 Window rows.  Rows 3-27 now contain
; sprite-HUD pattern pixels, not name-table words, so keep the Window hidden;
; the Stage Clear and bonus lettering is already sprite-based.
        org     ORIGIN_STAGE_CLEAR_WINDOW_IMM
        dc.w    $9200

        org     ORIGIN_LEVEL_WINDOW_IMM
        dc.w    $9200

        org     ORIGIN_GAME_FRAME
        jmp     HUD_GAME_FRAME
        nop

; COM VS and VS share a second battle loop.  Seed the fixed HUD records before
; either player creates dynamic sprites, just as the story loop does.
        org     ORIGIN_GAME2_FRAME
        jmp     HUD_GAME2_FRAME
        nop

; Replace the original Window-based pause display by blinking PAUSE through the
; five left SCORE patterns.  The complete sprite list remains frozen.
        org     ORIGIN_PAUSE_ENTRY
        jmp     HUD_PAUSE_ROUTINE

        org     ORIGIN_GAME1_WINDOW_IMM
        dc.w    $9200

        org     ORIGIN_GAME2_WINDOW_IMM
        dc.w    $9200

; Hide the Window only when the shared VS selection screen hands control to
; the battle engine.  The selection screen itself keeps its original setup.
        org     ORIGIN_VS_WINDOW_IMM
        dc.w    $9200

; The original VS result code uploads the two-tile round medal at $1980,
; which is now the left SCORE sprite's O column.  Redirect both winner paths
; to the VS-only $1800/$1820 tile pair.  The hidden Window makes its original
; $CC/$CD name-table references harmless; sprite records reproduce them.
        org     ORIGIN_VS_MEDAL_UPLOAD1_IMM
        dc.l    HUD_VS_MEDAL_VDP_COMMAND

        org     ORIGIN_VS_MEDAL_UPLOAD2_IMM
        dc.l    HUD_VS_MEDAL_VDP_COMMAND

; Preserve all logical HUD behavior while mirroring its cells into sprite art.
        org     ORIGIN_WINDOW_WRITE
        jmp     HUD_WINDOW_WRITE

        org     ORIGIN_WINDOW_BLANK
        jmp     HUD_WINDOW_BLANK

        org     ORIGIN_TIMEOUT_GFX
        jmp     HUD_TIMEOUT_GFX

; Three-button layout: A = heavy punch, B = light punch, C = guard.
        org     ORIGIN_BUTTON_A_MAPPING
        dc.b    BUTTON_HEAVY_PUNCH

        org     ORIGIN_BUTTON_B_MAPPING
        dc.b    BUTTON_LIGHT_PUNCH

        org     ORIGIN_BUTTON_C_MAPPING
        dc.b    BUTTON_GUARD

        org     ORIGIN_DASH_GUARD_HOOK
        jmp     HUD_DASH_GUARD

; Reset the authoritative RAM shadow before either gameplay mode draws its
; initial health bars.  The first hook replaces a true no-op call; the second
; uses a small wrapper for its displaced initialization instructions.
        org     ORIGIN_HUD_RESET_GAME1
        jsr     HUD_RESET_SHADOW

        org     ORIGIN_HUD_RESET_GAME2
        jmp     HUD_INIT_SHADOW_GAME2

        include "SRC/hud_code.inc"
