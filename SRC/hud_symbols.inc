; Mad Stalker - Full Metal Forth (Genesis, 2 MiB ROM)
; Window-HUD to sprite-HUD patch: fixed ROM entry points and code layout.

ORIGIN_VBLANK_TAIL            equ $000D63E6
ORIGIN_GAME_LINK_CALL         equ $000D6228
ORIGIN_WINDOW_CLEAR_A_COUNT   equ $000D8776
ORIGIN_WINDOW_CLEAR_B_COUNT   equ $000D8796
ORIGIN_DISPLAY_CLEAR_COUNT    equ $000FDB96
ORIGIN_SYSTEM_CLEAR_COUNT     equ $001339AC

ORIGIN_GAME_FRAME             equ $000FB156
ORIGIN_GAME_FRAME_CONTINUE    equ $000FB15E
ORIGIN_GAME_FRAME_SLOW        equ $000FB188
ORIGIN_GAME2_FRAME            equ $001286A2
ORIGIN_GAME2_FRAME_CONTINUE   equ $001286AA
ORIGIN_GAME2_FRAME_SLOW       equ $001286D4
ORIGIN_HUD_RESET_GAME1        equ $000FB104
ORIGIN_HUD_RESET_GAME2        equ $00128568
ORIGIN_HUD_RESET_GAME2_CONT   equ $0012856E

ORIGIN_PAUSE_ENTRY            equ $000FD006
ORIGIN_LEVEL_WINDOW_IMM       equ $000D8CCC
ORIGIN_GAME1_WINDOW_IMM       equ $0012652E
ORIGIN_GAME2_WINDOW_IMM       equ $001269EC
ORIGIN_VS_WINDOW_IMM          equ $00127BFC

ORIGIN_WINDOW_WRITE           equ $00133A58
ORIGIN_WINDOW_BLANK           equ $00133A86
ORIGIN_TIMEOUT_GFX            equ $00133DF8

ORIGIN_BUTTON_A_MAPPING       equ $00126CEF
ORIGIN_BUTTON_C_MAPPING       equ $00126CFB
ORIGIN_BUTTON_B_MAPPING       equ $00126D07
ORIGIN_DASH_GUARD_HOOK        equ $0011BA2A
ORIGIN_DASH_GUARD_CONTINUE    equ $0011BA34

SUB_SET_VRAM_WRITE            equ $0011D12A
SUB_LINK_SPRITES              equ $0011CEC8
SUB_PAUSE_ENTER               equ $001011F8
SUB_PAUSE_RESTORE             equ $0010129A
SUB_PAUSE_WAIT                equ $000FD16A

; The original ROM is zero-filled from $1FBD4A to EOF.
HUD_CODE_BASE                 equ $001FBD4A
HUD_GAME_FRAME                equ $001FBD4A
HUD_GAME2_FRAME               equ $001FBD70
HUD_WINDOW_WRITE              equ $001FBDC0
HUD_WINDOW_BLANK              equ $001FBE20
HUD_TIMEOUT_GFX               equ $001FBE70
HUD_VBLANK_TAIL               equ $001FBF40
HUD_REFRESH_DIRTY             equ $001FBF60
HUD_COPY_PATTERN              equ $001FBFC0
HUD_REFRESH_TIMER             equ $001FC0C0
HUD_REFRESH_ALL               equ $001FC120
HUD_RESET_SHADOW              equ $001FC180
HUD_INIT_SHADOW_GAME2         equ $001FC1D0
HUD_CELL_TABLE                equ $001FC200
HUD_SPRITE_TABLE              equ $001FC320
HUD_REFRESH_CELLS             equ $001FC3A0
HUD_MARK_CELL_DIRTY           equ $001FC440
HUD_CELL_INDEX                equ $001FC500
HUD_PREPARED_STATIC           equ $001FC560
HUD_PREPARED_TIMEOUT          equ $001FCCE0
HUD_PREPARED_END              equ $001FD1E0
HUD_SEED_SPRITES              equ $001FD1E0
HUD_LINK_SPRITES              equ $001FD240
HUD_PAUSE_ROUTINE             equ $001FD2A0
HUD_PREPARED_PAUSE            equ $001FD500
HUD_DASH_GUARD                equ $001FD9A0

BUTTON_HEAVY_PUNCH            equ $01
BUTTON_LIGHT_PUNCH            equ $02
BUTTON_GUARD                  equ $03

BUTTON_CHANGE_VAR             equ $FFFFB803
ACTION_VAR                    equ $FFFFC66C
DASH_VAR                      equ $FFFFDD3C
LIGHT_PUNCH_VAR               equ $FFFFC6F8
PLAYER_INDEX_VAR              equ $FFFFB900

SPRITE_TABLE_VRAM             equ $1000
PAUSE_SPRITE_VRAM             equ $1200
PAUSE_PATTERN_VRAM            equ $1280

; Address-shaped key retained from the original HUD layout.  It is used only
; to index the RAM shadow; patched HUD routines never access it as VRAM.
HUD_LOGICAL_MAP_BASE          equ $1800
SPRITE_TABLE_RAM              equ $FFFFD898
SPRITE_COUNT                  equ $FFFFB8C8
GAME_MODE                     equ $FFFFB818
HUD_DIRTY                     equ $FFFFF663
HUD_TIMER_RIGHT               equ $FFFFF680
HUD_TIMER_LEFT                equ $FFFFF681
HUD_TIMER_VALID               equ $FFFFF682
HUD_LINKS_VALID               equ $FFFFF683
HUD_CELL_DIRTY                equ $FFFFF684
HUD_CELL_VALID                equ $FFFFF68D
HUD_CELL_SHADOW               equ $FFFFF696
HUD_QUEUE_COUNT               equ $FFFFF70E
HUD_QUEUE_DATA                equ $FFFFF70F
