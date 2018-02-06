; .prg example of using the playroutine. Press fire to trigger a sound effect.

                processor 6502
                org $0801

sys:            dc.b $0b,$08
                dc.b $0a,$00
                dc.b $9e
                dc.b $32,$30,$36,$31
                dc.b $00,$00,$00

start:          sei
                lda #$00
                sta $d415
                lda #$7f
                sta $dc0d
                lda #$01
                sta $d01a
                lda #<raster
                sta $0314
                lda #>raster
                sta $0315
                lda #$33
                sta $d012
                lda #27
                sta $d011
                lda $dc0d
                lda $d019
                sta $d019
                cli
loop:           lda #68
                sta $0400
                sta $0428
                sta $0450
                ldx curraster
                lda hexchars,x
                sta $400+37
                lda #"/"
                sta $400+38
                ldx maxraster
                lda hexchars,x
                sta $400+39
                jmp loop

hexchars:       dc.b $30,$31,$32,$33,$34,$35,$36,$37,$38,$39
                dc.b $01,$02,$03,$04,$05,$06

curraster:      dc.b 0
maxraster:      dc.b 0

raster:         cld
                lda $d019
                sta $d019
                nop
                nop
                nop
                nop
                nop
                nop
                nop
                nop
                nop
                nop
                nop
                nop
                nop
                nop
                inc $d020
                jsr PlayRoutine
                lda $d012
                ldx #$0e
                stx $d020
                sec
                sbc #$34
                sta curraster
                cmp maxraster
                bcc raster_nonewmax
                sta maxraster
raster_nonewmax:lda $dc00
                pha
                and #$10
                bne raster_nofire
                lda prevjoy
                and #$10
                beq raster_nofire
                ldx #14
                lda #<soundeffect
                ldy #>soundeffect
                jsr PlaySfx
raster_nofire:  pla
                sta prevjoy
                jmp $ea31

prevjoy:        dc.b 0

                org $1000

        ; Player configuration

PLAYER_ZPBASE   = $fb
PLAYER_SFX      = 2

        ; Player + musicdata

                include player.s
                include musicdata.s

        ; SFX data

soundeffect:    dc.b $01                        ;Hardrestart
                dc.b $06,$00,$fa,$08            ;Init+ADSR+pulse
                dc.b $18,$81,$c0                ;Wave+note
                dc.b $18,$41,$0c                ;Wave+note
                dc.b $18,$81,$20                ;Wave+note
                dc.b $20,$ff                    ;Freqmod
                dc.b $08,$80                    ;Wave (gate off)
                dc.b $f0                        ;Delay for 16 frames
                dc.b $00                        ;End
