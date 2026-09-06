; ---------------------------------------------------------------------------
; Sprite HUD implementation
;
; HUD_WINDOW_WRITE/HUD_WINDOW_BLANK retain the original routine interfaces but
; store name words only in the authoritative RAM shadow.  No gameplay HUD map
; is written to or read from VRAM.  This keeps all original HUD logic, art,
; palette use, score/HP/timer behavior, and screen transitions.
; ---------------------------------------------------------------------------

        org     HUD_GAME_FRAME
HudGameplayFrame:
        movem.l d0-d1/a0-a1,-(sp)
        move.w  #$9200,(VDP_CTRL).l       ; register 18: no Window rows
        bsr.w   HudSeedStorySprites       ; Stage Clear intentionally starts at entry 0
        movem.l (sp)+,d0-d1/a0-a1

        ; Reproduce the two instructions replaced at $FB156.
        tst.l   (dword_FFB820).w
        bgt.s   HudGameplayContinue
        jmp     ORIGIN_GAME_FRAME_SLOW
HudGameplayContinue:
        jmp     ORIGIN_GAME_FRAME_CONTINUE

; COM VS and VS both run the alternate battle loop at $1286A2.  Its displaced
; instructions and branch shape match the story loop, but it needs its own
; continuation addresses.  Seeding before any battle objects draw reserves
; SAT entries 0-7 and lets both robots append complete dynamic sprite chains.
        org     HUD_GAME2_FRAME
HudGameplayFrame2:
        movem.l d0-d1/a0-a1,-(sp)
        move.w  #$9200,(VDP_CTRL).l       ; no Window rows during VS battle
        bsr.w   HudSeedSprites
        move.b  #HUD_FRAME_VS,(HUD_FRAME_SEEDED).w ; append round medals after objects
        movem.l (sp)+,d0-d1/a0-a1

        ; Reproduce the two instructions replaced at $1286A2.
        tst.l   (dword_FFB820).w
        bgt.s   HudGameplay2Continue
        jmp     ORIGIN_GAME2_FRAME_SLOW
HudGameplay2Continue:
        jmp     ORIGIN_GAME2_FRAME_CONTINUE

; RAM-only replacement for sub_133A58.  The original address calculation is
; retained as a logical cell key, but no VDP command or Window write is issued.
        org     HUD_WINDOW_WRITE
HudWindowWrite:
        link    a6,#0
        move.l  d0,-(sp)                  ; original routine preserves d0
        move.l  $10(a6),d1
        asl.l   #6,d1
        move.l  $C(a6),d2
        add.l   d2,d2
        add.w   d2,d1
        addi.w  #HUD_LOGICAL_MAP_BASE,d1
        move.w  d1,-(sp)
        move.w  $16(a6),d1
        addi.w  #-$7E80,d1                ; original: tile + $8180
        move.w  d1,d0                     ; d0 = new name-table word
        move.w  (sp)+,d1                  ; d1 = logical map address
        bsr.w   HudMarkCellDirty
        move.w  d0,d1                     ; preserve the original return value
        move.l  (sp)+,d0
        unlk    a6
        rts

; RAM-only replacement for sub_133A86.  Blank cells use the original $80A1
; name word in the shadow and become transparent when copied to the atlas.
        org     HUD_WINDOW_BLANK
HudWindowBlank:
        link    a6,#0
        move.l  d0,-(sp)                  ; original routine preserves d0
        move.l  $10(a6),d1
        asl.l   #6,d1
        move.l  $C(a6),d2
        add.l   d2,d2
        add.w   d2,d1
        addi.w  #HUD_LOGICAL_MAP_BASE,d1
        move.w  #$80A1,d0
        bsr.w   HudMarkCellDirty
        move.l  (sp)+,d0
        unlk    a6
        rts

; Replacement for sub_133DF8.  Prepared ROM graphics make the original two
; 128-byte source-VRAM DMAs unnecessary.  One DIVU obtains both digits, and
; only a four-tile digit whose value changed is marked for refresh.  Dirty bit
; 1 is the left/tens digit; bit 3 is the right/ones digit.
        org     HUD_TIMEOUT_GFX
HudTimeoutGraphics:
        move.l  d3,-(sp)
        move.l  8(sp),d3
        moveq   #$63,d1
        cmp.l   d3,d1
        bge.s   HudTimeoutClamped
        moveq   #$63,d3
HudTimeoutClamped:
        moveq   #0,d0
        move.w  d3,d0
        moveq   #10,d1
        divu.w  d1,d0                    ; quotient low, remainder high
        moveq   #0,d2
        move.w  d0,d2                    ; d2 = left/tens digit
        swap    d0
        andi.l  #$0000FFFF,d0            ; d0 = right/ones digit
        move.l  d2,(dword_FFB8D8).w      ; preserve original final scratch value

        cmpi.b  #$A5,(HUD_TIMER_VALID).w
        bne.s   HudTimeoutForceBoth
        cmp.b   (HUD_TIMER_LEFT).w,d2
        beq.s   HudTimeoutLeftReady
        ori.b   #2,(HUD_DIRTY).w
HudTimeoutLeftReady:
        cmp.b   (HUD_TIMER_RIGHT).w,d0
        beq.s   HudTimeoutStore
        ori.b   #8,(HUD_DIRTY).w
        bra.s   HudTimeoutStore
HudTimeoutForceBoth:
        ori.b   #$0A,(HUD_DIRTY).w
        move.b  #$A5,(HUD_TIMER_VALID).w
HudTimeoutStore:
        move.b  d2,(HUD_TIMER_LEFT).w
        move.b  d0,(HUD_TIMER_RIGHT).w
        moveq   #0,d0
        move.w  #$7780,d0                ; preserve original sub_133DF8 return value
        move.l  (sp)+,d3
        rts

; Reserve entries 0-7 in the game's normal 64-entry staging table.  Their
; immutable words normally survive the VBlank upload, so verify a signature
; and skip the 64-byte template copy on ordinary frames.  A screen transition
; that clears or reuses the staging table fails the signature, restores all
; records, and requests a guarded full atlas rebuild.
        org     HUD_SEED_SPRITES
HudSeedSprites:
        lea     (SPRITE_TABLE_RAM).w,a1
        cmpi.w  #$0080,(a1)
        bne.s   HudSeedCopy
        cmpi.w  #$80C6,4(a1)
        bne.s   HudSeedCopy
        cmpi.w  #$0088,$38(a1)
        bne.s   HudSeedCopy
        cmpi.w  #$80FF,$3C(a1)
        bne.s   HudSeedCopy
        cmpi.w  #$0170,$3E(a1)
        beq.s   HudSeedCounts
HudSeedCopy:
        lea     (HudSpriteTable,pc),a0
        moveq   #$0F,d0                   ; 16 longs = 8 sprite records
HudSeedCopyLoop:
        move.l  (a0)+,(a1)+
        dbf     d0,HudSeedCopyLoop
        ori.b   #1,(HUD_DIRTY).w
        clr.b   (HUD_LINKS_VALID).w
HudSeedCounts:
        moveq   #8,d0
        move.l  d0,(SPRITE_COUNT).w
        move.w  d0,(dword_FFE44C).w        ; uploader reads the high word here
        st      (HUD_FRAME_SEEDED).w        ; authorize one HUD-aware link pass
        rts

; Gameplay-only fast sprite linker.  A one-use marker distinguishes an
; ordinary HUD frame from Stage Clear, cutscenes, and menus that share the
; global original linker call.  After the original routine has initialized
; the restored template once, links 0-6 are invariant and remain untouched.
; Entry 7 links to gameplay entry 8 only when one exists; entries 8 onward are
; rebuilt normally.  During Stage Clear, append the two bonus values before
; finalizing the chain because their original Window-row writes are suppressed.
; Counts below the eight reserved HUD records use the original general-purpose
; routine as a guarded fallback.
        org     HUD_LINK_SPRITES
HudLinkSprites:
        bsr.w   HudLinkFramePrepare      ; consumes the per-frame seed marker
        tst.b   d0
        beq.s   HudLinkSpritesNonGameplay
        cmpi.b  #$A5,(HUD_LINKS_VALID).w
        bne.s   HudLinkSpritesOriginal
        move.l  4(sp),d0
        cmpi.w  #8,d0
        bcs.s   HudLinkSpritesOriginal
        lea     (SPRITE_TABLE_RAM+$3B).w,a0 ; entry 7 link byte
        cmpi.w  #8,d0
        bne.s   HudLinkSpritesGameplay
        clr.b   (a0)                       ; HUD-only list ends at entry 7
        rts
HudLinkSpritesGameplay:
        move.b  #8,(a0)                   ; entry 7 -> first gameplay sprite
        adda.w  #8,a0                     ; entry 8 link byte
        subi.w  #9,d0
        beq.s   HudLinkSpritesLast         ; exactly one gameplay sprite
        subq.w  #1,d0                     ; DBF count for entries 8..N-2
        moveq   #9,d1
HudLinkSpritesLoop:
        move.b  d1,(a0)
        addq.b  #1,d1
        addq.l  #8,a0
        dbf     d0,HudLinkSpritesLoop
HudLinkSpritesLast:
        clr.b   (a0)
        rts
HudLinkSpritesOriginal:
        move.b  #$A5,(HUD_LINKS_VALID).w
        jmp     (SUB_LINK_SPRITES).l        ; preserve the caller's argument frame
HudLinkSpritesNonGameplay:
        clr.b   (HUD_LINKS_VALID).w         ; never carry fast-link state across modes
        jmp     (SUB_LINK_SPRITES).l        ; rebuild complete Stage Clear/cutscene/menu chain

; Replaces the original D61FA epilogue.  By this point the game's dynamic VRAM
; cache has finished, so score/HP/timer source patterns are stable.
        org     HUD_VBLANK_TAIL
HudVBlankTail:
        tst.b   (byte_FFF484).w            ; freeze HUD pattern work while paused
        bne.s   HudVBlankTailDone
        tst.b   (HUD_DIRTY).w
        beq.s   HudVBlankTailDone
        bsr.w   HudRefreshDirty
HudVBlankTailDone:
        movem.l (sp)+,d3/a3-a6
        rts

; Dirty bit 0 requests all 60 cells, bit 1 the left timer digit, bit 2 only
; map cells recorded in HUD_CELL_DIRTY, and bit 3 the right timer digit.
; Bit 4 schedules the one-time Stage 5 panel upload.  Service that larger
; transfer by itself and retain any ordinary HUD bits for the next VBlank.
; Ordinary HUD writes use bit 2 so a score/HP change cannot trigger a costly
; full-atlas rebuild every frame.
        org     HUD_REFRESH_DIRTY
HudRefreshDirty:
        move.l  d0,-(sp)
        moveq   #0,d0
        move.b  (HUD_DIRTY).w,d0
        beq.s   HudRefreshDirtyDone
        clr.b   (HUD_DIRTY).w
        btst    #4,d0
        beq.s   HudRefreshDirtyRegular
        andi.b  #HUD_DIRTY_REGULAR_MASK,d0
        or.b    d0,(HUD_DIRTY).w           ; defer simultaneous ordinary HUD work
        bsr.w   HudStage5PanelUpload
        bra.s   HudRefreshDirtyDone
HudRefreshDirtyRegular:
        btst    #0,d0
        beq.s   HudRefreshDirtyPartial
        bsr.w   HudRefreshAll
        bra.s   HudRefreshDirtyDone
HudRefreshDirtyPartial:
        btst    #1,d0
        beq.s   HudRefreshTimerRightTest
        bsr.w   HudRefreshTimerLeft
HudRefreshTimerRightTest:
        btst    #3,d0
        beq.s   HudRefreshTimerHandledTest
        bsr.w   HudRefreshTimerRight
HudRefreshTimerHandledTest:
        move.w  d0,d1
        andi.w  #$000A,d1
        beq.s   HudRefreshDirtyCells
        btst    #2,d0
        beq.s   HudRefreshDirtyDone
        ori.b   #4,(HUD_DIRTY).w           ; defer unrelated cells one VBlank
        bra.s   HudRefreshDirtyDone
HudRefreshDirtyCells:
        btst    #2,d0
        beq.s   HudRefreshDirtyDone
        bsr.w   HudRefreshCells
HudRefreshDirtyDone:
        move.l  (sp)+,d0
        rts

; d4.w = destination VRAM byte address, d5.w = source name-table word.
; The HUD's original resources are duplicated at build time with opaque color
; 1 already changed to transparent color 0.  Copying from ROM avoids the old
; VRAM read, stack buffer, per-long black-pixel conversion, and CRAM scan.
; $A1 is the game's blank Window tile and becomes a zero-filled sprite tile.
        org     HUD_COPY_PATTERN
HudCopyPattern:
        lea     (VDP_DATA).l,a0
        lea     (VDP_CTRL).l,a1

        ; Build and retain the VRAM-write command for d4.
        moveq   #0,d6
        move.w  d4,d6
        move.w  d4,d0
        andi.w  #$3FFF,d6
        ori.w   #$4000,d6
        swap    d6
        rol.w   #2,d0
        andi.w  #3,d0
        move.w  d0,d6
        movea.l d6,a2

        move.w  d5,d0
        andi.w  #$07FF,d0
        cmpi.w  #$00A1,d0
        beq.s   HudCopyBlank
        subi.w  #$0180,d0                ; logical HUD tile ID
        bcs.s   HudCopyBlank
        cmpi.w  #$003B,d0                ; static IDs $00-$3B
        bls.s   HudCopyStatic
        cmpi.w  #$003F,d0                ; ones digit IDs $3C-$3F
        bls.s   HudCopyOnes
        cmpi.w  #$0043,d0                ; tens digit IDs $40-$43
        bhi.s   HudCopyBlank

        subi.w  #$0040,d0
        moveq   #0,d1
        move.b  (HUD_TIMER_RIGHT).w,d1
        bra.s   HudCopyTimer
HudCopyOnes:
        subi.w  #$003C,d0
        moveq   #0,d1
        move.b  (HUD_TIMER_LEFT).w,d1
HudCopyTimer:
        cmpi.w  #9,d1
        bls.s   HudCopyTimerValid
        moveq   #0,d1                    ; guarded initial-state fallback
HudCopyTimerValid:
        lsl.w   #7,d1                    ; digit -> 128-byte resource
        lsl.w   #5,d0                    ; tile within the four-tile digit
        add.w   d1,d0
        lea     (HudPreparedTimeout,pc),a3
        adda.w  d0,a3
        bra.s   HudCopyPrepared

HudCopyStatic:
        lsl.w   #5,d0
        lea     (HudPreparedStatic,pc),a3
        adda.w  d0,a3
HudCopyPrepared:
        move.l  a2,(a1)
        moveq   #7,d3
HudCopyPreparedLoop:
        move.l  (a3)+,(a0)
        dbf     d3,HudCopyPreparedLoop
        rts

HudCopyBlank:
        move.l  a2,(a1)
        moveq   #0,d0
        move.l  d0,(a0)
        move.l  d0,(a0)
        move.l  d0,(a0)
        move.l  d0,(a0)
        move.l  d0,(a0)
        move.l  d0,(a0)
        move.l  d0,(a0)
        move.l  d0,(a0)
        rts

; The timeout's visible digits occupy entries 25-32.  Their name-table tile
; IDs are fixed, so copy directly from the prepared digit library without the
; previous Window-VRAM name-table reads.  Each call updates only four tiles.
        org     HUD_REFRESH_TIMER
HudRefreshTimerLeft:
        movem.l d0-d7/a0-a4,-(sp)
        lea     (HudCellTable+$64,pc),a4
        move.w  #$81BC,d5
        bra.s   HudRefreshTimerReady
HudRefreshTimerRight:
        movem.l d0-d7/a0-a4,-(sp)
        lea     (HudCellTable+$74,pc),a4
        move.w  #$81C0,d5
HudRefreshTimerReady:
        moveq   #3,d7
HudRefreshTimerLoop:
        move.w  2(a4),d4
        adda.w  #4,a4
        bsr.w   HudCopyPattern
        addq.w  #1,d5
        dbf     d7,HudRefreshTimerLoop
        movem.l (sp)+,d0-d7/a0-a4
        rts

; Full rebuild is needed only after a screen transition invalidates the atlas.
; Source name words come exclusively from the RAM shadow.  Invalid cells are
; treated as blank.  All 60 visible HUD cells are rebuilt.
        org     HUD_REFRESH_ALL
HudRefreshAll:
        movem.l d0-d7/a0-a4,-(sp)
        clr.l   (HUD_CELL_DIRTY).w
        clr.l   (HUD_CELL_DIRTY+4).w
        clr.b   (HUD_CELL_DIRTY+8).w
        clr.b   (HUD_QUEUE_COUNT).w
        lea     (HudCellTable,pc),a4
        moveq   #0,d2                    ; RAM-shadow/table index
        moveq   #$3B,d7                   ; 60 table entries
HudRefreshAllLoop:
        addq.l  #2,a4                    ; skip the logical cell key
        move.w  (a4)+,d4                 ; destination pattern byte address
        move.w  d2,d0
        move.w  d2,d1
        andi.w  #7,d0
        lsr.w   #3,d1
        lea     (HUD_CELL_VALID).w,a0
        btst    d0,(a0,d1.w)
        beq.s   HudRefreshAllBlank
        move.w  d2,d0
        add.w   d0,d0
        lea     (HUD_CELL_SHADOW).w,a0
        move.w  (a0,d0.w),d5
        bra.s   HudRefreshAllCopy
HudRefreshAllBlank:
        move.w  #$80A1,d5
HudRefreshAllCopy:
        bsr.w   HudCopyPattern
        addq.w  #1,d2
        dbf     d7,HudRefreshAllLoop
        movem.l (sp)+,d0-d7/a0-a4
        rts

; Initialize the complete RAM shadow to the original blank word and invalidate
; pending work.  Transition-time cost is small and there are no VDP accesses.
        org     HUD_RESET_SHADOW
HudResetShadow:
        movem.l d0-d1/a0,-(sp)
        lea     (HUD_CELL_SHADOW).w,a0
        move.w  #$80A1,d0
        moveq   #$3B,d1                   ; 60 logical cells
HudResetShadowLoop:
        move.w  d0,(a0)+
        dbf     d1,HudResetShadowLoop
        ; HUD_CELL_VALID starts at an odd address.  Byte access to the first
        ; flag is legal, then the long/word stores use even addresses.  This
        ; preserves the packed RAM layout without triggering a 68000 address
        ; error on hardware-accurate emulators.
        moveq   #-1,d1
        move.b  d1,(HUD_CELL_VALID).w
        move.l  d1,(HUD_CELL_VALID+1).w
        move.w  d1,(HUD_CELL_VALID+5).w
        move.b  #$0F,(HUD_CELL_VALID+7).w
        clr.l   (HUD_CELL_DIRTY).w
        clr.l   (HUD_CELL_DIRTY+4).w
        clr.b   (HUD_CELL_DIRTY+8).w
        clr.b   (HUD_QUEUE_COUNT).w
        clr.b   (HUD_TIMER_VALID).w
        ; A warm reset preserves the staging table, then the original reset
        ; code replaces only every sprite's size/link word with $0501.  Clear
        ; entry 0's checked Y word so HudSeedSprites cannot accept that damaged
        ; table as the fixed HUD template.  HudSeedCopy restores all eight
        ; records and clears HUD_LINKS_VALID on the first gameplay frame.
        clr.w   (SPRITE_TABLE_RAM).w
        move.b  #1,(HUD_DIRTY).w           ; rebuild blank/current RAM state once
        movem.l (sp)+,d0-d1/a0
        tst.w   d0
        rts

; The alternate gameplay mode has no replaceable no-op immediately before its
; initial health calls.  Reset at the start of its setup block, restore the
; VS medal patterns after the three compatibility rows were cleared, then
; reproduce the displaced clr/moveq pair.
        org     HUD_INIT_SHADOW_GAME2
HudInitShadowGame2:
        bsr.w   HudResetShadow
        bsr.w   HudLoadVsMedalArt
        clr.b   (byte_FFE048).w
        moveq   #0,d7
        jmp     ORIGIN_HUD_RESET_GAME2_CONT

; Each pair is: logical HUD cell key, sprite pattern byte address.
; Pattern storage uses protected hidden Window rows.  The right HUD is laid
; out column-major so two 4x3 sprites plus one edge sprite cover score and HP.
        org     HUD_CELL_TABLE
HudCellTable:
        ; left 4x3
        dc.w $1802,$18C0, $1842,$18E0, $1882,$1900
        dc.w $1804,$1920, $1844,$1940, $1884,$1960
        dc.w $1806,$1980, $1846,$19A0, $1886,$19C0
        dc.w $1808,$19E0, $1848,$1A00, $1888,$1A20
        ; left 4x3
        dc.w $180A,$1A40, $184A,$1A60, $188A,$1A80
        dc.w $180C,$1AA0, $184C,$1AC0, $188C,$1AE0
        dc.w $180E,$1B00, $184E,$1B20, $188E,$1B40
        dc.w $1810,$1B60, $1850,$1B80, $1890,$1BA0
        ; left edge + visible centered 4x2 timeout.  Each 4x2 sprite stores tiles in
        ; column-major order: top-left, bottom-left, then the next column.
        dc.w $1852,$1BC0
        dc.w $185C,$1BE0, $189C,$1C00
        dc.w $185E,$1C20, $189E,$1C40
        dc.w $1860,$1C60, $18A0,$1C80
        dc.w $1862,$1CA0, $18A2,$1CC0
        ; right 4x3
        dc.w $182C,$1CE0, $186C,$1D00, $18AC,$1D20
        dc.w $182E,$1D40, $186E,$1D60, $18AE,$1D80
        dc.w $1830,$1DA0, $1870,$1DC0, $18B0,$1DE0
        dc.w $1832,$1E00, $1872,$1E20, $18B2,$1E40
        ; right 4x3
        dc.w $1834,$1E60, $1874,$1E80, $18B4,$1EA0
        dc.w $1836,$1EC0, $1876,$1EE0, $18B6,$1F00
        dc.w $1838,$1F20, $1878,$1F40, $18B8,$1F60
        dc.w $183A,$1F80, $187A,$1FA0, $18BA,$1FC0
        ; right edge + HI label (separate protected two-tile slot)
        dc.w $187C,$1FE0
        dc.w $1826,$17C0, $1828,$17E0

; Raw Genesis SAT records: Y, size/link, attributes, X.  Coordinates include
; the hardware +$80 bias.  Priority is set so HUD sprites stay above Plane A.
        org     HUD_SPRITE_TABLE
HudSpriteTable:
        dc.w $0080,$0E00,$80C6,$0088       ; left 4x3 at (8,0)
        dc.w $0080,$0E00,$80D2,$00A8       ; left 4x3 at (40,0)
        dc.w $0088,$0000,$80DE,$00C8       ; left edge at (72,8)
        dc.w $0088,$0D00,$80DF,$00F0       ; visible timeout 4x2 at (112,8)
        dc.w $0080,$0400,$80BE,$0118       ; HI label 2x1 at (152,0)
        dc.w $0080,$0E00,$80E7,$0130       ; right 4x3 at (176,0)
        dc.w $0080,$0E00,$80F3,$0150       ; right 4x3 at (208,0)
        dc.w $0088,$0000,$80FF,$0170       ; right edge at (240,8)

; d0.w = new name-table word, d1.w = logical HUD cell key.  Convert the
; address directly to one of 96 row/column slots, then use HudCellIndex to get
; its atlas entry in O(1) time.  $FF denotes a cell not represented by sprites.
        org     HUD_MARK_CELL_DIRTY
HudMarkCellDirty:
        movem.l d0-d4/a0-a1,-(sp)
        move.w  d1,d3
        subi.w  #HUD_LOGICAL_MAP_BASE,d3
        bcs.w   HudMarkCellDone
        btst    #0,d3
        bne.w   HudMarkCellDone
        lsr.w   #1,d3
        cmpi.w  #$005F,d3
        bhi.w   HudMarkCellDone
        lea     (HudCellIndex,pc),a0
        moveq   #0,d4
        move.b  (a0,d3.w),d4
        cmpi.b  #$FF,d4
        beq.w   HudMarkCellDone
        move.w  d4,d3
        lsr.w   #3,d3
        move.w  d4,d2
        andi.w  #7,d2
        lea     (HUD_CELL_VALID).w,a0
        btst    d2,(a0,d3.w)
        beq.s   HudMarkCellChanged
        move.w  d4,d1
        add.w   d1,d1
        lea     (HUD_CELL_SHADOW).w,a1
        cmp.w   (a1,d1.w),d0
        beq.s   HudMarkCellDone
HudMarkCellChanged:
        bset    d2,(a0,d3.w)
        move.w  d4,d1
        add.w   d1,d1
        lea     (HUD_CELL_SHADOW).w,a1
        move.w  d0,(a1,d1.w)
        lea     (HUD_CELL_DIRTY).w,a0
        bset    d2,(a0,d3.w)
        bne.s   HudMarkCellSignal          ; already represented in the queue
        moveq   #0,d1
        move.b  (HUD_QUEUE_COUNT).w,d1
        cmpi.w  #$003C,d1
        bcc.s   HudMarkQueueOverflow
        lea     (HUD_QUEUE_DATA).w,a0
        move.b  d4,(a0,d1.w)
        addq.b  #1,(HUD_QUEUE_COUNT).w
HudMarkCellSignal:
        ori.b   #4,(HUD_DIRTY).w
        bra.s   HudMarkCellDone
HudMarkQueueOverflow:
        ori.b   #1,(HUD_DIRTY).w           ; guarded fallback: rebuild all cells
HudMarkCellDone:
        movem.l (sp)+,d0-d4/a0-a1
        rts

; Direct logical-cell -> HudCellTable index map.  Three rows of 32 bytes retain
; the original $1800-$18BF address shape only as RAM lookup keys.
        org     HUD_CELL_INDEX
HudCellIndex:
        ; row 0: $1800-$183F
        dc.b $FF,$00,$03,$06,$09,$0C,$0F,$12
        dc.b $15,$FF,$FF,$FF,$FF,$FF,$FF,$FF
        dc.b $FF,$FF,$FF,$3A,$3B,$FF,$21,$24
        dc.b $27,$2A,$2D,$30,$33,$36,$FF,$FF
        ; row 1: $1840-$187F
        dc.b $FF,$01,$04,$07,$0A,$0D,$10,$13
        dc.b $16,$18,$FF,$FF,$FF,$FF,$19,$1B
        dc.b $1D,$1F,$FF,$FF,$FF,$FF,$22,$25
        dc.b $28,$2B,$2E,$31,$34,$37,$39,$FF
        ; row 2: $1880-$18BF
        dc.b $FF,$02,$05,$08,$0B,$0E,$11,$14
        dc.b $17,$FF,$FF,$FF,$FF,$FF,$1A,$1C
        dc.b $1E,$20,$FF,$FF,$FF,$FF,$23,$26
        dc.b $29,$2C,$2F,$32,$35,$38,$FF,$FF

; Consume only queued changed cells, with an eight-tile VBlank budget.  The
; remaining queue is compacted and carried into the next frame.
        org     HUD_REFRESH_CELLS
HudRefreshCells:
        movem.l d0-d7/a0-a4,-(sp)
        moveq   #0,d7
        move.b  (HUD_QUEUE_COUNT).w,d7
        beq.s   HudRefreshCellsDone
        cmpi.w  #8,d7
        bls.s   HudRefreshQueueCountReady
        moveq   #8,d7
HudRefreshQueueCountReady:
        subq.w  #1,d7
        lea     (HUD_QUEUE_DATA).w,a4
HudRefreshQueueLoop:
        moveq   #0,d6
        move.b  (a4)+,d6
        move.w  d6,d0
        move.w  d6,d1
        andi.w  #7,d0
        lsr.w   #3,d1
        lea     (HUD_CELL_DIRTY).w,a3
        bclr    d0,(a3,d1.w)
        move.w  d6,d0
        lsl.w   #2,d0
        lea     (HudCellTable,pc),a0
        move.w  2(a0,d0.w),d4
        move.w  d6,d0
        add.w   d0,d0
        lea     (HUD_CELL_SHADOW).w,a0
        move.w  (a0,d0.w),d5
        bsr.w   HudCopyPattern
        subq.b  #1,(HUD_QUEUE_COUNT).w
        dbf     d7,HudRefreshQueueLoop

        moveq   #0,d7
        move.b  (HUD_QUEUE_COUNT).w,d7
        beq.s   HudRefreshCellsDone
        lea     (HUD_QUEUE_DATA).w,a0
        subq.w  #1,d7
HudRefreshQueueCompact:
        move.b  (a4)+,(a0)+
        dbf     d7,HudRefreshQueueCompact
        ori.b   #4,(HUD_DIRTY).w           ; continue remaining entries next VBlank
HudRefreshCellsDone:
        movem.l (sp)+,d0-d7/a0-a4
        rts

; Build-generated, uncompressed 4bpp HUD graphics.  The first $780 bytes are
; static tile IDs $00-$3B; the following $500 bytes are digits 0-9, four tiles
; per digit.  The original ROM resources remain byte-for-byte untouched.
        org     HUD_PREPARED_STATIC
HudPreparedStatic:
        incbin  "../ASSETS/hud_prepared.bin"
HudPreparedTimeout equ HudPreparedStatic+$0780

; ---------------------------------------------------------------------------
; Sprite-HUD pause display
;
; The original pause routine replaced HUD-atlas patterns and exposed a Window
; strip.  This replacement blocks in the same way but leaves the Window hidden
; and temporarily replaces the five left SCORE tiles with PAUSE.  It never
; changes the SAT, so all HUD/gameplay sprite records and links remain frozen
; and the display stays within the H32 hardware limit of entries 0-63.
; ---------------------------------------------------------------------------
        org     HUD_PAUSE_ROUTINE
HudPauseRoutine:
        btst    #7,(word_FFB802+1).w
        beq.w   HudPauseReturn
        tst.l   (dword_FFBA10).w
        bne.w   HudPauseReturn
        tst.l   (dword_FFBA18).w
        bne.w   HudPauseReturn
        tst.l   (dword_FFBA1C).w
        bne.w   HudPauseReturn

        st      (byte_FFF484).w
        jsr     (SUB_PAUSE_ENTER).l
        bsr.w   HudPauseLabelShow
        move.w  #$9100,(VDP_CTRL).l
        move.w  #$9200,(VDP_CTRL).l       ; Window remains fully hidden
        move.l  #HUD_PAUSE_VISIBLE_FRAMES,-(sp)
HudPauseLoop:
        jsr     (SUB_PAUSE_WAIT).l
        btst    #7,(word_FFB802+1).w
        bne.s   HudPauseExit

        ; The low word of this stack-local counter is positive while PAUSE is
        ; visible and negative while it is blank.  Keeping the state local to
        ; this blocking routine guarantees that no blink can survive resume.
        tst.w   2(sp)
        bmi.s   HudPauseBlinkHidden
        subq.w  #1,2(sp)
        bne.s   HudPauseLoop
        bsr.w   HudPauseLabelBlank
        move.w  #-HUD_PAUSE_HIDDEN_FRAMES,2(sp)
        bra.s   HudPauseLoop
HudPauseBlinkHidden:
        addq.w  #1,2(sp)
        bne.s   HudPauseLoop
        bsr.w   HudPauseLabelShow
        move.w  #HUD_PAUSE_VISIBLE_FRAMES,2(sp)
        bra.s   HudPauseLoop

HudPauseExit:
        addq.l  #4,sp                    ; discard pause-local blink counter
        bsr.w   HudPauseLabelHide
        jsr     (SUB_PAUSE_RESTORE).l
        clr.b   (byte_FFF484).w
        move.w  #$9100,(VDP_CTRL).l
        move.w  #$9200,(VDP_CTRL).l
HudPauseReturn:
        rts

; SCORE occupies five unique top-row patterns across fixed HUD entries 0-1.
; Replace only those patterns; the player score and health rows below them use
; different tiles and remain visible.  The PAUSE asset already uses transparent
; index 0 and foreground index $C, which is valid through the HUD palette line.
HudPauseLabelShow:
        movem.l d0-d7/a0-a2,-(sp)
        lea     (HudPreparedPause,pc),a0
        lea     (HudPauseScoreDestinations,pc),a1
        moveq   #4,d7                    ; five contiguous PAUSE tiles
        bsr.s   HudPauseCopyTiles
        movem.l (sp)+,d0-d7/a0-a2
        rts

; Blank only the five PAUSE/SCORE pattern cells.  A small zero-fill loop avoids
; another graphics asset and leaves every other HUD/gameplay pattern untouched.
HudPauseLabelBlank:
        movem.l d0-d7/a0-a2,-(sp)
        lea     (HudPauseScoreDestinations,pc),a1
        lea     (VDP_DATA).l,a2
        moveq   #4,d7                    ; five tiles
HudPauseBlankTile:
        move.w  (a1)+,d1
        jsr     (SUB_SET_VRAM_WRITE).l
        moveq   #0,d2                    ; do not rely on the helper preserving d2
        moveq   #7,d0                    ; eight transparent longs per tile
HudPauseBlankLong:
        move.l  d2,(a2)
        dbf     d0,HudPauseBlankLong
        dbf     d7,HudPauseBlankTile
        movem.l (sp)+,d0-d7/a0-a2
        rts

; Static tile IDs $1B4-$1B7 are S,C,O,R, while $1B8 is T and $1BB is E.
; Restore the four contiguous letters first, then the non-contiguous E.  This
; repairs only the five overwritten cells without requesting a full atlas
; rebuild or disturbing pending HUD updates.
HudPauseLabelHide:
        movem.l d0-d7/a0-a2,-(sp)
        lea     (HudPreparedStatic+$0680,pc),a0
        lea     (HudPauseScoreDestinations,pc),a1
        moveq   #3,d7                    ; S,C,O,R
        bsr.s   HudPauseCopyTiles
        lea     (HudPreparedStatic+$0760,pc),a0
        moveq   #0,d7                    ; E; a1 now selects the fifth cell
        bsr.s   HudPauseCopyTiles
        movem.l (sp)+,d0-d7/a0-a2
        rts

; a0 = contiguous source tiles, a1 = destination-address list, d7 = count - 1.
; The destination patterns are separated by the 4x3 sprites' column-major
; layout, so issue one VRAM command per tile.
HudPauseCopyTiles:
        lea     (VDP_DATA).l,a2
HudPauseCopyTile:
        move.w  (a1)+,d1
        jsr     (SUB_SET_VRAM_WRITE).l
        moveq   #7,d0                    ; eight longs = one 8x8 tile
HudPauseCopyLong:
        move.l  (a0)+,(a2)
        dbf     d0,HudPauseCopyLong
        dbf     d7,HudPauseCopyTile
        rts

HudPauseScoreDestinations:
        dc.w    $18C0,$1920,$1980,$19E0,$1A40

; Build-generated duplicate of the original five PAUSE tiles.  Palette index
; 1 is made transparent only in this private copy; the source ROM art is kept.
; During pause these tiles temporarily replace the five left SCORE patterns.
        org     HUD_PREPARED_PAUSE
HudPreparedPause:
        incbin  "../ASSETS/hud_pause_prepared.bin"

; ---------------------------------------------------------------------------
; Stage Clear bonus values
;
; The labels are dynamic sprites, but the original eight-digit values use
; sub_133AD4 to write Window rows 16 and 22.  Those addresses overlap the
; protected sprite-HUD atlas and the Window remains hidden, so reproduce the
; same calculations as one-tile digit sprites.  The game's contiguous digit
; font at tile $1A8 is already loaded for the original Window display.
;
; Story Stage Clear deliberately skips the eight fixed HUD records so its
; original dynamic labels begin at SAT entry 0.  VS never calls this story-only
; selector.  The distinct marker lets HudLinkFramePrepare append the bonus
; values, then select the original full linker for the resulting zero-based
; list.  d7.l enters with the current SAT count and returns with the appended
; count.
; ---------------------------------------------------------------------------
        org     HUD_STAGE_CLEAR_NUMBERS
HudSeedStorySprites:
        move.l  (dword_FFBA1C).w,d0
        beq.w   HudSeedSprites            ; ordinary story gameplay keeps the HUD
        ; The original frame loop enters Stage Clear only when BA1C is active
        ; and the BA18 death/Game Over state machine is idle.  Check BA18 only
        ; on this nonzero event path, preserving the ordinary gameplay timing.
        tst.l   (dword_FFBA18).w
        bne.w   HudSeedSprites            ; death/Game Over is not Stage Clear
        cmpi.l  #$BA,d0
        bcc.w   HudSeedSprites            ; Stage Clear ended; restore normal seeding

        moveq   #0,d0
        move.l  d0,(SPRITE_COUNT).w       ; Stage Clear labels start at entry 0
        move.w  d0,(dword_FFE44C).w       ; uploader count starts at zero as well
        move.b  #HUD_FRAME_STAGE_CLEAR,(HUD_FRAME_SEEDED).w
        rts

HudLinkFramePrepare:
        moveq   #0,d0
        move.b  (HUD_FRAME_SEEDED).w,d0
        beq.s   HudLinkFrameReady         ; cutscene/menu: original full linker
        clr.b   (HUD_FRAME_SEEDED).w       ; marker is valid for one link pass
        cmpi.b  #HUD_FRAME_STAGE_CLEAR,d0
        beq.s   HudLinkFrameStageClear
        cmpi.b  #HUD_FRAME_VS,d0
        beq.s   HudLinkFrameVs

        moveq   #1,d0                     ; ordinary story HUD fast linker
        rts

HudLinkFrameVs:
        ; The helper return and the linker's caller return precede the count.
        ; Saving d7 moves that count argument to 12(sp).  Medals are appended
        ; after every robot/effect record, so their variable count cannot shift
        ; or hide gameplay sprites.
        move.l  d7,-(sp)
        move.l  12(sp),d7
        bsr.w   HudVsMedalAppend
        move.l  d7,12(sp)
        move.l  d7,(SPRITE_COUNT).w
        move.w  d7,(dword_FFE44C).w       ; SAT uploader count
        move.l  (sp)+,d7
        moveq   #1,d0                     ; retain the gameplay HUD fast linker
        rts

HudLinkFrameStageClear:
        move.l  (dword_FFBA1C).w,d0
        cmpi.l  #$41,d0                  ; CLEAR BONUS has been calculated
        bcs.s   HudLinkFrameStageReady
        cmpi.l  #$BA,d0                  ; transition begins after state $B9
        bcc.s   HudLinkFrameStageReady

        ; The helper return and the linker's caller return precede the count.
        ; Saving d7 moves that count argument to 12(sp).
        move.l  d7,-(sp)
        move.l  12(sp),d7
        bsr.w   HudStageClearAppend
        move.l  d7,12(sp)
        move.l  d7,(SPRITE_COUNT).w
        move.w  d7,(dword_FFE44C).w      ; SAT uploader count
        move.l  (sp)+,d7
HudLinkFrameStageReady:
        moveq   #0,d0                    ; original linker handles entry 0 onward
HudLinkFrameReady:
        rts

HudStageClearAppend:
        movem.l d0-d6/a0-a1,-(sp)
        cmpi.w  #$0040,d7                ; H32 SAT has 64 entries
        bcc.s   HudStageClearAppendDone

        move.w  d7,d0
        lsl.w   #3,d0                    ; eight bytes per SAT record
        lea     (SPRITE_TABLE_RAM).w,a0
        adda.w  d0,a0
        move.l  (dword_FFBA1C).w,d6

        ; CLEAR BONUS = remaining-time units * 1000, at Window row 16.
        move.l  (dword_FFB9E4).w,d0
        move.w  #1000,d1
        mulu.w  d1,d0
        move.w  #$0100,d4                ; screen Y 128 + SAT bias $80
        move.w  #$0108,d5                ; rightmost column 17 + bias
        bsr.s   HudStageClearRenderValue

        ; LEVEL BONUS appears only on the original non-negative stage path.
        cmpi.l  #$42,d6
        bcs.s   HudStageClearAppendDone
        move.l  (dword_FFB8AC).w,d0
        bmi.s   HudStageClearAppendDone
        addq.l  #1,d0
        move.w  #5000,d1
        mulu.w  d1,d0
        move.w  #$0130,d4                ; screen Y 176 + SAT bias $80
        move.w  #$0108,d5
        bsr.s   HudStageClearRenderValue

HudStageClearAppendDone:
        movem.l (sp)+,d0-d6/a0-a1
        rts

; Render a positive or zero 32-bit value right-aligned in the original
; eight-cell field.  Leading zero cells need no sprites because the background
; is already transparent.  Inputs: d0.l value, d4.w Y, d5.w rightmost X,
; a0 next SAT record, d7.w count.  Returns updated a0/d7.
HudStageClearRenderValue:
        moveq   #10,d1
        moveq   #7,d3                    ; original field is eight cells wide
HudStageClearDigitLoop:
        cmpi.w  #$0040,d7
        bcc.s   HudStageClearRenderDone
        divu.w  d1,d0                    ; quotient low, remainder high
        move.l  d0,d2
        swap    d2
        andi.w  #$000F,d2
        addi.w  #$81A8,d2               ; priority + digit tile 0
        move.w  d4,(a0)+
        clr.w   (a0)+                    ; 1x1 size; linker fills link byte
        move.w  d2,(a0)+
        move.w  d5,(a0)+
        addq.w  #1,d7
        subi.w  #8,d5
        andi.l  #$0000FFFF,d0            ; retain quotient for next digit
        beq.s   HudStageClearRenderDone
        dbf     d3,HudStageClearDigitLoop
HudStageClearRenderDone:
        rts

; Append the original VS round-win medals after the dynamic sprite list.
; dword_FFD64C is 1P's win count and dword_FFD650 is 2P's.  The match ends
; when either reaches two, but clamp both counters defensively.  d7.w enters
; and returns as the SAT record count; a full table drops medals before any
; gameplay sprite can be displaced.
HudVsMedalAppend:
        movem.l d0-d3/a0,-(sp)
        cmpi.w  #$0040,d7                ; H32 SAT has 64 entries
        bcc.s   HudVsMedalAppendDone

        move.w  d7,d0
        lsl.w   #3,d0                    ; eight bytes per SAT record
        lea     (SPRITE_TABLE_RAM).w,a0
        adda.w  d0,a0
        moveq   #2,d2                    ; best-of-three: two wins end the match

        move.l  (dword_FFD64C).w,d0      ; 1P medals begin at screen (88,8)
        cmp.l   d2,d0
        bls.s   HudVsMedal1PCountReady
        move.l  d2,d0
HudVsMedal1PCountReady:
        move.w  #$00D8,d1                ; X 88 + hardware bias $80
        bsr.s   HudVsMedalRenderSide

        move.l  (dword_FFD650).w,d0      ; 2P medals begin at screen (160,8)
        cmp.l   d2,d0
        bls.s   HudVsMedal2PCountReady
        move.l  d2,d0
HudVsMedal2PCountReady:
        move.w  #$0120,d1                ; X 160 + hardware bias $80
        bsr.s   HudVsMedalRenderSide

HudVsMedalAppendDone:
        movem.l (sp)+,d0-d3/a0
        rts

; d0.l = medal count, d1.w = first biased X, a0 = next SAT record.
; One 1x2 sprite reuses the same two tile patterns for every medal.  Successive
; wins move eight pixels right, exactly matching the original Window address
; increment.  d7.w/a0 return advanced.
HudVsMedalRenderSide:
        tst.l   d0
        ble.s   HudVsMedalRenderDone
        subq.w  #1,d0                    ; DBF count
HudVsMedalRenderLoop:
        cmpi.w  #$0040,d7
        bcc.s   HudVsMedalRenderDone
        move.w  #$0088,(a0)+             ; screen Y 8 + hardware bias $80
        move.w  #$0100,(a0)+             ; 1x2 size; linker supplies link byte
        move.w  #$8000+HUD_VS_MEDAL_TILE,(a0)+ ; priority, palette 0, tiles C0/C1
        move.w  d1,(a0)+
        addq.w  #8,d1
        addq.w  #1,d7
        dbf     d0,HudVsMedalRenderLoop
HudVsMedalRenderDone:
        rts

; The compatibility Window rows include VRAM $1800-$18BF and are cleared on
; every VS round transition.  Reload the original uncompressed two-tile medal
; once after that clear.  The original winner paths are also redirected here,
; so neither path can overwrite the SCORE atlas at $1980/$19A0.
HudLoadVsMedalArt:
        movem.l d0/a0-a1,-(sp)
        move.l  #HUD_VS_MEDAL_VDP_COMMAND,(VDP_CTRL).l
        lea     (ORIGIN_VS_MEDAL_ART).l,a0
        lea     (VDP_DATA).l,a1
        moveq   #$0F,d0                  ; 16 longs = two 8x8 tiles
HudLoadVsMedalLoop:
        move.l  (a0)+,(a1)
        dbf     d0,HudLoadVsMedalLoop
        movem.l (sp)+,d0/a0-a1
        rts

; Story Stage 3 scene 0 scrolls Plane B vertically by 32 pixels.  Its visible
; screen rows 0-23 therefore map to tile rows $484-$486, not the map's first
; three rows.  Extend the original upper orange/index-$5 and dark-orange/
; index-$4 dither unit through screen lines 0-6 (7 scanlines), with dither
; line 3 shifted horizontally by one pixel.  Keep lines 1 and 5 in the
; original phase and lines 7-23 solid
; orange.  Extend that orange band two more scanlines into tile
; $487 and move $487's original first three lower-transition scanlines down by
; the same two lines.  The remaining lower gradient scanlines stay exactly
; where the original stage placed them.  Stage 3 CRAM index $4 is $024C.
;
; This runs once after the normal Stage 3 background DMA has completed.  It
; does not change CRAM, the Plane B map, any other stage, or the per-frame
; gameplay path.  HudResetShadow remains last so its original return flags are
; preserved for the setup caller.
        org     HUD_INIT_SHADOW_GAME1
HudInitShadowGame1:
        bsr.s   HudStage3SolidHudBand
        jmp     (HudResetShadow).l

HudStage3SolidHudBand:
        cmpi.l  #3,(dword_FFB814).w
        bne.s   HudStage3SolidHudBandDone
        tst.l   (dword_FFB9C4).w          ; setup normalizes scene 0 to state $00
        bne.s   HudStage3SolidHudBandDone ; later Stage 3 resumes use state $0B
        movem.l d0-d2/a0,-(sp)
        move.w  #$8F02,(VDP_CTRL).l       ; VDP auto-increment = two bytes
        move.l  #HUD_STAGE3_SKY_VDP_COMMAND,(VDP_CTRL).l
        lea     (VDP_DATA).l,a0
        move.l  #$55555555,d0             ; eight pixels of palette index 5
        move.l  #$45454545,d1             ; original index 4/5 dither phase
        move.l  d0,(a0)                   ; line 0: solid orange
        move.l  d1,(a0)                   ; line 1: original dither phase
        move.l  d0,(a0)                   ; line 2: solid orange
        ror.l   #4,d1                      ; shift line 3 one pixel
        move.l  d1,(a0)                   ; line 3: one-pixel phase shift
        move.l  d0,(a0)                   ; line 4: solid orange
        ror.l   #4,d1                      ; restore original phase for line 5
        move.l  d1,(a0)                   ; line 5: original dither phase
        move.l  d0,(a0)                   ; line 6 completes 7-line band
        moveq   #$10,d2                   ; lines 7-23: solid orange
HudStage3SolidHudBandLoop:
        move.l  d0,(a0)
        dbf     d2,HudStage3SolidHudBandLoop

        ; Tile $487 lines 0-4: two-line orange extension followed by its
        ; original first three transition lines, shifted down two scanlines.
        move.l  d0,(a0)
        move.l  d0,(a0)
        move.l  #$56565656,(a0)
        move.l  #$66666666,(a0)
        move.l  #$65656565,(a0)
        movem.l (sp)+,d0-d2/a0
HudStage3SolidHudBandDone:
        rts

; The Stage 5 boss-room transition reaches this original call only once, after
; the room's Plane A patterns have been installed.  Preserve the displaced
; transition signal and schedule the panel transfer for VBlank.  Prepared
; copies remap only palette-0 index $D to existing index $B; borders and every
; other pixel are byte-for-byte identical.
        org     HUD_STAGE5_PANEL_SETUP
HudStage5PanelSetup:
        jsr     (SUB_STAGE5_TRANSITION_SIGNAL).l
        ori.b   #HUD_DIRTY_STAGE5_PANEL,(HUD_DIRTY).w
        rts

HudStage5PanelUpload:
        movem.l d0/a0-a1,-(sp)
        move.w  #$8F02,(VDP_CTRL).l       ; VDP auto-increment = two bytes
        lea     (HudStage5PanelTiles680).l,a0
        lea     (VDP_DATA).l,a1
        move.l  #HUD_STAGE5_PANEL_680_COMMAND,(VDP_CTRL).l
        moveq   #$27,d0                  ; 40 longs = tiles $680-$684
HudStage5Panel680Loop:
        move.l  (a0)+,(a1)
        dbf     d0,HudStage5Panel680Loop

        lea     (HudStage5PanelTiles6A8).l,a0
        move.l  #HUD_STAGE5_PANEL_6A8_COMMAND,(VDP_CTRL).l
        moveq   #$17,d0                  ; 24 longs = tiles $6A8-$6AA
HudStage5Panel6A8Loop:
        move.l  (a0)+,(a1)
        dbf     d0,HudStage5Panel6A8Loop
        movem.l (sp)+,d0/a0-a1
        rts

        even
HudStage5PanelTiles680:
        ; Tiles $680-$684.
        dc.l    $13FBBBBB,$1EFBBBBB,$1EFBBBBB,$1EFBBBBB
        dc.l    $1EFBBBBB,$13FBBBBB,$13FBBBBB,$13FBBBBB
        dc.l    $11111111,$3E33EEE3,$FFFFFFFF,$BBBBBBBB
        dc.l    $BBBBBBBB,$BBBBBBBB,$BBBBBBBB,$BBBBBBBB
        dc.l    $BBBBBBBB,$BBBBBBBB,$BBBBBBBB,$BBBBBBBB
        dc.l    $BBBBBBBB,$BBBBBBBB,$BBBBBBBB,$BBBBBBBB
        dc.l    $11111111,$333EEEE1,$FFFFFFE1,$BBBBB6E1
        dc.l    $BBBBB6E1,$BBBBB6E1,$BBBBB6E1,$BBBBB631
        dc.l    $BBBBB631,$BBBBB6E1,$BBBBB6E1,$BBBBB6E1
        dc.l    $BBBBB631,$BBBBB631,$BBBBB6E1,$BBBBB6E1

HudStage5PanelTiles6A8:
        ; Tiles $6A8-$6AA.
        dc.l    $1EFBBBBB,$1EFBBBBB,$1EFBBBBB,$13FBBBBB
        dc.l    $1EFBBBBB,$1EF66666,$1EEEEE33,$11111111
        dc.l    $BBBBBBBB,$BBBBBBBB,$BBBBBBBB,$BBBBBBBB
        dc.l    $BBBBBBBB,$66666666,$33EE3EE3,$11111111
        dc.l    $BBBBB6E1,$BBBBB6E1,$BBBBB6E1,$BBBBB631
        dc.l    $BBBBB6E1,$666666E1,$33333EE1,$11111111

; Three-button guard compatibility helper.  Reproduce the displaced dash-state
; copy, then preserve the original patch's guard+B behavior.  The standalone
; patch indexed ACTION_VAR with an unscaled player number, which makes player
; 2 perform an odd-address longword read.  Scale once here so all per-player
; longword accesses remain aligned on a real 68000 and Genesis Plus GX.
        org     HUD_DASH_GUARD
HudDashGuard:
        lea     (DASH_VAR).w,a0
        move.b  (a1,d0.l),(a0,d0.l)

        move.l  (PLAYER_INDEX_VAR).w,d0
        asl.l   #2,d0
        lea     (ACTION_VAR).w,a0
        cmpi.l  #BUTTON_GUARD,(a0,d0.l)
        bne.s   HudDashGuardReturn

        moveq   #0,d1
        move.b  (BUTTON_CHANGE_VAR).w,d1
        rol.b   #4,d1
        andi.l  #7,d1
        btst    #1,d1
        beq.s   HudDashGuardReturn

        lea     (LIGHT_PUNCH_VAR).w,a0
        move.l  #1,(a0,d0.l)
HudDashGuardReturn:
        jmp     ORIGIN_DASH_GUARD_CONTINUE
