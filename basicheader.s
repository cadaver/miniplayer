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
raster_nonewmax:jmp $ea31

                org $1000

        ; Player configuration

PLAYER_ZPBASE   = $fc

        ; Player + musicdata

                include player.s
                include musicdata.s
