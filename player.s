; Minimal feature-limited C64 music player
; Written by Cadaver 2/2018

TRANS           = $80
SONGJUMP        = 0

ENDPATT         = 0
INS             = -1
DUR             = $101
C0              = 1*2+1
CS0             = 2*2+1
D0              = 3*2+1
DS0             = 4*2+1
E0              = 5*2+1
F0              = 6*2+1
FS0             = 7*2+1
G0              = 8*2+1
GS0             = 9*2+1
A0              = 10*2+1
AS0             = 11*2+1
H0              = 12*2+1
C1              = 13*2+1
CS1             = 14*2+1
D1              = 15*2+1
DS1             = 16*2+1
E1              = 17*2+1
F1              = 18*2+1
FS1             = 19*2+1
G1              = 20*2+1
GS1             = 21*2+1
A1              = 22*2+1
AS1             = 23*2+1
H1              = 24*2+1
C2              = 25*2+1
CS2             = 26*2+1
D2              = 27*2+1
DS2             = 28*2+1
E2              = 29*2+1
F2              = 30*2+1
FS2             = 31*2+1
G2              = 32*2+1
GS2             = 33*2+1
A2              = 34*2+1
AS2             = 35*2+1
H2              = 36*2+1
C3              = 37*2+1
CS3             = 38*2+1
D3              = 39*2+1
DS3             = 40*2+1
E3              = 41*2+1
F3              = 42*2+1
FS3             = 43*2+1
G3              = 44*2+1
GS3             = 45*2+1
A3              = 46*2+1
AS3             = 47*2+1
H3              = 48*2+1
C4              = 49*2+1
CS4             = 50*2+1
D4              = 51*2+1
DS4             = 52*2+1
E4              = 53*2+1
F4              = 54*2+1
FS4             = 55*2+1
G4              = 56*2+1
GS4             = 57*2+1
A4              = 58*2+1
AS4             = 59*2+1
H4              = 60*2+1
WAVEPTR         = $7c
KEYOFF          = $7e
REST            = $7f

trackPtrLo      = $fc
trackPtrHi      = $fd
pattPtrLo       = $fe
pattPtrHi       = $ff

Play_DoInit:    dex
                txa
                sta pattPtrLo
                asl
                asl
                adc pattPtrLo
                tay
                lda songTbl,y
                sta trackPtrLo
                iny
                lda songTbl,y
                sta trackPtrHi
                iny
                ldx #$00
                stx Play_FiltPos+1
                stx Play_FiltType+1
                stx $d417
                stx PlayRoutine+1
                jsr Play_ChnInit
                ldx #$07
                jsr Play_ChnInit
                ldx #$0e
Play_ChnInit:   lda songTbl,y
                sta chnSongPos,x
                iny
                ;lda #$ff
                lda #$fb
                sta chnDuration,x
                lda #$00
                sta chnCounter,x
                sta chnNote,x
                sta chnPattPos,x
                sta chnWavePos,x
                sta chnPulsePos,x
                sta $d404,x
                rts

Play_FiltInit:  lda filtSpdTbl-$81,y
                sta $d417
                and #$30
                sta Play_FiltType+1
                lda filtNextTbl-$81,y
                sta Play_FiltPos+1
                lda filtLimitTbl-$81,y
                jmp Play_StoreCutoff
Play_FiltNext:  lda filtNextTbl-1,y
                sta Play_FiltPos+1
                jmp Play_FiltDone

Play_Commands:  cmp #KEYOFF
                beq Play_Keyoff
                bcs Play_Rest
Play_WavePtr:   iny
                lda (pattPtrLo),y
                jsr Play_SetWavePos
                beq Play_Rest                   ;Returns with A=0

PlayRoutine:    ldx #$01
                bne Play_DoInit
Play_FiltPos:   ldy #$00
                beq Play_FiltDone
                bmi Play_FiltInit
Play_FiltCutoff:lda #$00
                cmp filtLimitTbl-1,y
                beq Play_FiltNext
                clc
                adc filtSpdTbl-1,y
Play_StoreCutoff:
                sta Play_FiltCutoff+1
                sta $d416
Play_FiltDone:
Play_FiltType:  lda #$00
Play_MasterVol: ora #$0f
                sta $d418
                jsr Play_ChnExec
                ldx #$07
                jsr Play_ChnExec
                ldx #$0e

Play_ChnExec:   inc chnCounter,x
                bne Play_NoNewNotes
Play_NewNotes:  ldy chnSongPos,x                ;Assume sequencer has had time to advance to next pattern
                lda (trackPtrLo),y
                tay
                lda pattTblLo-1,y
                sta pattPtrLo
                lda pattTblHi-1,y
                sta pattPtrHi
                ldy chnPattPos,x
                lda (pattPtrLo),y
Play_NoNewDur:  cmp #WAVEPTR
                bcs Play_Commands
                lsr
                bcs Play_NoNewIns
                adc chnTrans,x
                ora #$80
                sta chnNote,x
                iny
                lda (pattPtrLo),y
                sta chnIns,x
                bmi Play_Rest                   ;Instruments $80-$ff are $00-$7f as legato
Play_HardRestart:
                lda #$0f
                sta $d406,x
Play_Keyoff:    lda chnWave,x
                and #$fe
                sta $d404,x
Play_Rest:      iny
                lda (pattPtrLo),y
                beq Play_PattEnd
                bpl Play_NoPattEnd
                sta chnDuration,x
                iny
                lda (pattPtrLo),y
                bne Play_NoPattEnd
Play_PattEnd:   sta chnPattPos,x
                inc chnSongPos,x
                rts

Play_NoPattEnd: tya
                sta chnPattPos,x
                rts

Play_NoNewIns:  clc
                adc chnTrans,x
                ora #$80
                sta chnNote,x
                lda chnIns,x
                bpl Play_HardRestart
                bmi Play_Rest

Play_LegatoNoteInit:
                tya
                and #$7f
                tay
                bpl Play_FinishLegatoInit

Play_NoNewNotes:bmi Play_PulseExec
                lda chnDuration,x
                sta chnCounter,x
                lda chnPattPos,x
                bne Play_NoSequencer

Play_Sequencer: ldy chnSongPos,x
                lda (trackPtrLo),y
                beq Play_SongJump
                bmi Play_SongTrans
                bpl Play_SequencerDone
Play_SongJump:  iny
                lda (trackPtrLo),y
                tay
                lda (trackPtrLo),y
                bpl Play_NoSongJumpTrans
Play_SongTrans: sta chnTrans,x
                iny
Play_NoSongJumpTrans:
                tya
                sta chnSongPos,x
Play_SequencerDone:
                lda chnNote,x
                bpl Play_WaveExec

Play_NewNoteInit:
                and #$7f
                sta chnNote,x                   ;Reset newnote-flag
                ldy chnIns,x
                bmi Play_LegatoNoteInit
Play_HRNoteInit:lda insAD-1,y
                sta $d405,x
                lda insSR-1,y
                sta $d406,x
                lda #$09                        ;Fixed 1stframe wave
                sta $d404,x
Play_FinishLegatoInit:
                lda insPulsePos-1,y
                beq Play_SkipPulseInit
                sta chnPulsePos,x
Play_SkipPulseInit:
                lda insFiltPos-1,y
                beq Play_SkipFiltInit
                sta Play_FiltPos+1
Play_SkipFiltInit:
                lda insWavePos-1,y
Play_SetWavePos:sta chnWavePos,x
                lda #$00
                sta chnWaveTime,x
                rts

Play_NoSequencer:
                lda chnNote,x
                bmi Play_NewNoteInit

Play_PulseExec: ldy chnPulsePos,x
                beq Play_WaveExec
                bmi Play_PulseInit
                lda chnPulse,x
                cmp pulseLimitTbl-1,y
                beq Play_PulseNext
                clc
                adc pulseSpdTbl-1,y
                adc #$00
Play_StorePulse:
                sta chnPulse,x
                sta $d402,x
                sta $d403,x

Play_WaveExec:  ldy chnWavePos,x
                beq Play_WaveDone
                lda waveTbl-1,y
                beq Play_Vibrato
                cmp #$90
                bcc Play_SetWave
                beq Play_Slide
Play_WaveDelay: adc chnWaveTime,x
                bne Play_WaveDelayNotOver
                clc
                sta chnWaveTime,x
                bcc Play_NoWaveChange

Play_PulseInit: lda pulseNextTbl-$81,y
                sta chnPulsePos,x
                lda pulseLimitTbl-$81,y
                jmp Play_StorePulse
Play_PulseNext: lda pulseNextTbl-1,y
                sta chnPulsePos,x
                jmp Play_WaveExec

Play_SetWave:   sta chnWave,x
                sta $d404,x
Play_NoWaveChange:
                lda waveNextTbl-1,y
                sta chnWavePos,x
                lda noteTbl-1,y
                bmi Play_AbsNote
                adc chnNote,x
Play_AbsNote:   asl
                tay
                lda freqTbl-2,y
                sta chnFreqLo,x
                sta $d400,x
                lda freqTbl-1,y
Play_StoreFreqHi:
                sta chnFreqHi,x
                sta $d401,x
Play_WaveDone:  rts
Play_WaveDelayNotOver:
                inc chnWaveTime,x
                rts

Play_Slide:     lda chnFreqLo,x
                clc
                adc noteTbl-1,y
                sta chnFreqLo,x
                sta $d400,x
                lda chnFreqHi,x
                adc waveNextTbl-1,y
                jmp Play_StoreFreqHi

Play_Vibrato:   lda chnWaveTime,x
                bpl Play_VibNoDir
                cmp noteTbl-1,y
                bcs Play_VibNoDir2
                eor #$ff
Play_VibNoDir:  sec
Play_VibNoDir2: sbc #$02
                sta chnWaveTime,x
                lsr
                lda chnFreqLo,x
                bcs Play_VibDown
Play_VibUp:     adc waveNextTbl-1,y
                sta chnFreqLo,x
                sta $d400,x
                bcc Play_WaveDone
                lda chnFreqHi,x
                adc #$00
                jmp Play_StoreFreqHi
Play_VibDown:   sbc waveNextTbl-1,y
                sta chnFreqLo,x
                sta $d400,x
                bcs Play_WaveDone
                lda chnFreqHi,x
                sbc #$00
                jmp Play_StoreFreqHi

chnTrans:       dc.b $80
chnSongPos:     dc.b 0
chnPattPos:     dc.b 0
chnDuration:    dc.b 0
chnCounter:     dc.b 0
chnNote:        dc.b 0
chnIns:         dc.b 0

                dc.b $80,0,0,0,0,0,0
                dc.b $80,0,0,0,0,0,0

chnWave:        dc.b 0
chnWavePos:     dc.b 0
chnWaveTime:    dc.b 0
chnPulse:       dc.b 0
chnPulsePos:    dc.b 0
chnFreqLo:      dc.b 0
chnFreqHi:      dc.b 0

                dc.b 0,0,0,0,0,0,0
                dc.b 0,0,0,0,0,0,0

freqTbl:
                dc.w $022d,$024e,$0271,$0296,$02be,$02e8,$0314,$0343,$0374,$03a9,$03e1,$041c
                dc.w $045a,$049c,$04e2,$052d,$057c,$05cf,$0628,$0685,$06e8,$0752,$07c1,$0837
                dc.w $08b4,$0939,$09c5,$0a5a,$0af7,$0b9e,$0c4f,$0d0a,$0dd1,$0ea3,$0f82,$106e
                dc.w $1168,$1271,$138a,$14b3,$15ee,$173c,$189e,$1a15,$1ba2,$1d46,$1f04,$20dc
                dc.w $22d0,$24e2,$2714,$2967,$2bdd,$2e79,$313c,$3429,$3744,$3a8d,$3e08,$41b8
                dc.w $45a1,$49c5,$4e28,$52cd,$57ba,$5cf1,$6278,$6853,$6e87,$751a,$7c10,$8371
                dc.w $8b42,$9389,$9c4f,$a59b,$af74,$b9e2,$c4f0,$d0a6,$dd0e,$ea33,$f820,$ffff
