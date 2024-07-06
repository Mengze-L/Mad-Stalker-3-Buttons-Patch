
ORIGIN_CODE_BUTTON_A_MAPPING          set $00126CEF
ORIGIN_CODE_BUTTON_B_MAPPING          set $00126D07
ORIGIN_CODE_BUTTON_C_MAPPING          set $00126CFB

ORIGIN_BUTTON_CHANGE_VAR              set $00FFB803

ORIGIN_ACTION_VAR                     set $00FFC66C

ORIGIN_DASH_VAR                       set $00FFDD3C
ORIGIN_HEAVY_PUNCH_VAR                set $00FFC6C0
ORIGIN_LIGHT_PUNCH_VAR                set $00FFC6F8
ORIGIN_OFFSET_VAR                     set $00FFB900

ORIGIN_CODE_DASH_SET                  set $0011BA2A
ORIGIN_CODE_DASH_RETURN               set $0011BA34

; Constants: -----------------------------------------------------------
HEAVY_PUNCH:                          equ $01
LIGHT_PUNCH:                          equ $02
GUARD:                                equ $03

; Overrides: -----------------------------------------------------------
        org     ORIGIN_CODE_BUTTON_A_MAPPING
        dc.b    HEAVY_PUNCH

        org     ORIGIN_CODE_BUTTON_B_MAPPING
        dc.b    LIGHT_PUNCH

        org     ORIGIN_CODE_BUTTON_C_MAPPING
        dc.b    GUARD

        org     ORIGIN_CODE_DASH_SET
        jmp     DASH_GUARD

; Change: ---------------------------------------------------------------
        org     $001FD9A0
DASH_GUARD
        lea     (ORIGIN_DASH_VAR).w,A0
        move.b  (A1,D0.l),(A0,D0.l)
        lea     (ORIGIN_ACTION_VAR).w,A0
        cmpi.l  #3,(A0,D0.l)
        bne.w   JUMP_ORIGIN
        move.b  (ORIGIN_BUTTON_CHANGE_VAR).w,D0
        rol.b   #4,D0
        andi.l  #7,D0
        btst    #1,D0
        beq.w   JUMP_ORIGIN
        lea     (ORIGIN_LIGHT_PUNCH_VAR),A0
        move.l  (ORIGIN_OFFSET_VAR).w,D0
        asl.l   #2,D0
        move.l  #1,(A0,D0.l)
JUMP_ORIGIN
        jmp     ORIGIN_CODE_DASH_RETURN
