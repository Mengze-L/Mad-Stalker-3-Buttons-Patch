
ORIGIN_BUTTON_A_MAP                   set $00126CEF
ORIGIN_BUTTON_B_MAP                   set $00126D07
ORIGIN_BUTTON_C_MAP                   set $00126CFB

; Constants: -----------------------------------------------------------
HEAVY_PUNCH:                          equ $01
LIGHT_PUNCH:                          equ $02
GUARD:                                equ $03

; Overrides: -----------------------------------------------------------
        org     ORIGIN_BUTTON_A_MAP
        dc.b    HEAVY_PUNCH

        org     ORIGIN_BUTTON_B_MAP
        dc.b    LIGHT_PUNCH

        org     ORIGIN_BUTTON_C_MAP
        dc.b    GUARD

; Change: ---------------------------------------------------------------
