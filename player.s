; Minimal feature-limited C64 music player
; Written by Cadaver (loorni@gmail.com) 2/2018

        ; Configuration:
        ;
        ; PLAYER_ZPBASE is the zeropage base address. 4 consecutive locations are needed,
        ; or 5 with sound effect support
        ;
        ; PLAYER_SFX is whether to compile in the sound FX player. Three possible values:
        ;       0 No sound FX support.
        ;       1 Overrides the music channel on which sounds are played, music can not
        ;         continue underneath.
        ;       2 Music is able to continue underneath and resume when the sound stops.
        ;         This has the effect of requiring extra channel variables and potentially
        ;         more rastertime.
        ;
        ; PLAYER_MODULES is whether the player can be pointed to new music modules during
        ; runtime (e.g. loadable music.) In this mode, the SetMusicData routine should be
        ; called before playback to set the music module address. Note that the music data
        ; itself needs to be assembled to a fixed known address, so that trackdata & pattern
        ; addresses are correct within their respective tables.

trackPtrLo      = PLAYER_ZPBASE+0
trackPtrHi      = PLAYER_ZPBASE+1
pattPtrLo       = PLAYER_ZPBASE+2
pattPtrHi       = PLAYER_ZPBASE+3

                if PLAYER_SFX > 0
sfxTemp         = PLAYER_ZPBASE+4
                endif

        ; Music module header format for SetMusicData: (written out first by the converter)
        ;
        ; - Song table size
        ; - Number of patterns
        ; - Number of instruments
        ; - Number of wave table steps
        ; - Number of pulse table steps
        ; - Number of filter table steps

MUSICHEADERSIZE = 6

FIXUP_SONGSIZE  = 0
FIXUP_PATTSIZE  = 4
FIXUP_INSSIZE   = 8
FIXUP_WAVESIZE  = 12
FIXUP_PULSESIZE = 16
FIXUP_FILTSIZE  = 20
FIXUP_NOSIZE    = $80

FIXUP_ZERO      = 0
FIXUP_MINUS1    = 1
FIXUP_MINUS81   = 2

NUMFIXUPS       = 29

        ; Track-data format:
        ; [transpose], pattern, [end]
        ;
        ; $00       Song end, followed by loop position
        ; $01-$7f   Patterns
        ; $80-$ff   Signed transpose, $80 being neutral
        ;
        ; All tracks of subtune must fit into 255 bytes. Start indices into trackdata are 1-based,
        ; as index 0 is used for PLAYER_SFX mode 1.

TRANS           = $80
SONGJUMP        = 0

        ; Pattern-data format:
        ; note, [instrument], [duration], [end]
        ;
        ; Note values:
        ; $00       Pattern end
        ; $02-$7a   Notes with instrument byte following
        ; $03-$7b   Notes without instrument change
        ; $7c       Waveptr change command, followed by new wave pointer
        ; $7e       Gate off
        ; $7f       Rest
        ;
        ; Duration values are negative from $80-$ff, with $fe representing the shortest legal
        ; duration (3)

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

        ; Instruments are roughly as in NinjaTracker 2, having ADSR and wave/pulse/filterpointers
        ;
        ; Instruments $01-$7f use gateoff before note and ADSR init.
        ; Instruments $81-$ff are the same instruments in legato mode: no gateoff, no ADSR change
        ;
        ; Use 0 in pulse & filterpointers to skip initialization.

        ; Tables are based on having 3 colums, like NinjaTracker 1.
        ; The right side column is the "next" position, 0 to stop wave/pulse/filter
        ;
        ; Wavetable left column values:
        ; $00       Vibrato, mid column is negative width, and right column speed
        ; $01-$8f   Waveform values, mid column is note ($00-$7f relative, $80-$ff absolute)
        ; $90       Slide, mid+right columns are 16bit speed-1, optimization due to carry being set
        ;           in slide routine
        ; $91-$ff   Delayed wavetable step without wavechange, delay is negative ($ff = one frame)
        ;
        ; If pulse or filterpointer has the high bit ($80) set, the next step will be interpreted
        ; as an init-step:
        ;
        ; For pulse: nybble-reversed initial pulse value in left (limit) column
        ; For filter: initial cutoff value in left column, mid column contains control bits:
        ;           $01,$02,$04     channels to filter
        ;           $10,$20         lowpass or bandpass (highpass not supported)
        ;           $40,$80,$c0     resonance control
        ;
        ; Otherwise pulse & filter steps include the cutoff/pulse target in the left column
        ; (for pulse, nybbles reversed) and speed (nybble reversed also) in the mid column.
        ; Negative pulse speed values must have one subtracted from them, ie. if you have speed 
        ; $40 up, use $bf for down with same speed.

                if PLAYER_MODULES > 0

        ; Point player to new music module
        ;
        ; A = music data address lowbyte
        ; X = music data address highbyte
        ;
        ; Playroutine should not be executed while this routine is executing, as they share the
        ; same zeropage locations.
        
SetMusicData:   sta SetMusicData_HeaderLda+1
                clc
                adc #MUSICHEADERSIZE
                sta trackPtrLo
                stx SetMusicData_HeaderLda+2
                txa
                adc #$00
                sta trackPtrHi
                ldx #NUMFIXUPS-1
SetMusicData_FixupLoop:
                lda fixupDestLoTbl,x
                sta pattPtrLo
                lda fixupDestHiTbl,x
                sta pattPtrHi
                lda fixupTypeTbl,x
                pha
                bmi SetMusicData_AddDone
                lsr
                lsr
                tay
SetMusicData_HeaderLda:
                lda musicHeader,y
                clc
                adc trackPtrLo
                sta trackPtrLo
                bcc SetMusicData_AddDone
                inc trackPtrHi
SetMusicData_AddDone:
                pla
                and #$03
                tay
                lda trackPtrLo
                sec
                sbc fixupSubTbl,y
                ldy #$01
                sta (pattPtrLo),y
                iny
                lda trackPtrHi
                sbc #$00
                sta (pattPtrLo),y
                dex
                bpl SetMusicData_FixupLoop
                rts

                endif

                if PLAYER_SFX > 0

        ; Sound effect playback init
        ;
        ; X = channel variable index (0,7,14)
        ; A = sound data address lowbyte
        ; Y = sound data address highbyte
        ; 
        ; Note: you must devise your own sound effect priority / interruption mechanism.
        ; Check the channel variable chnSfxPtrHi whether a sound is still playing (nonzero if is)
        ;
        ; Sound effect playback does not potentially work right if the call is interrupted by
        ; the playroutine. Therefore either protect it with sei/cli or call from the same 
        ; interrupt that calls the playroutine (recommended.)
        ;
        ; Sound effect data consists of control bytes followed by data, delays and endmark.
        ; Each control byte takes 1 frame to execute. The bits are evaluated from LSB first.
        ;
        ; $00       Endmark
        ; $01-$3f   Control byte, bits:
        ;               $01 Gateoff+HR, also resets internal variables. No other bits will be read.
        ;               $02 First frame init, followed by AD,SR bytes
        ;               $04 Set pulse, followed by nybble-reversed pulsewidth
        ;               $08 Set wave, followed by waveform byte
        ;               $10 Set frequency, followed by freq highbyte (also set to lowbyte)
        ;               $20 Set freqmod, followed by 8-bit freqmod speed, which affects only highbyte
        ; $80-$ff   Delay, is negative similar to wavetable ($ff = one frame)

PlaySfx:        sta chnSfxPtrLo,x
                tya
                sta chnSfxPtrHi,x
                lda #$00
                sta chnSfxPos,x
                if PLAYER_SFX = 1               ;Disable the channel for music playback until
                sta chnSongPos,x                ;new subtune initialized
                endif
                rts

                endif

        ; Playroutine entrypoint.
        ;
        ; Write subtune number+1 to PlayRoutine+1 to initialize

PlayRoutine:    ldx #$01
                beq Play_FiltPos
                jmp Play_DoInit

Play_FiltInit:
Play_FiltSpdTblM81Access:
                lda filtSpdTbl-$81,y
                sta $d417
                and #$30                        ;Lowpass or bandpass allowed, rest of bits used for
                sta Play_FiltType+1             ;resonance control
Play_FiltNextTblM81Access:
                lda filtNextTbl-$81,y
                sta Play_FiltPos+1
Play_FiltLimitTblM81Access:
                lda filtLimitTbl-$81,y
                jmp Play_StoreCutoff
Play_FiltNext:
Play_FiltNextTblM1Access:
                lda filtNextTbl-1,y
                sta Play_FiltPos+1
                jmp Play_FiltDone

Play_NoNewIns:  clc
                adc chnTrans,x
                ora #$80
                sta chnNote,x
                lda chnIns,x
                bpl Play_HardRestart
                jmp Play_Rest

Play_Commands:  cmp #KEYOFF
                if PLAYER_SFX = 2
                beq Play_KeyoffSfxCheck
                else
                beq Play_Keyoff
                endif
                beq Play_Keyoff
                bcs Play_Rest
Play_WavePtr:   iny
                lda (pattPtrLo),y
                jsr Play_SetWavePos
                beq Play_Rest                   ;Returns with A=0
                if PLAYER_SFX = 2
Play_KeyoffSfxCheck:
                lda chnSfxPtrHi,x
                bne Play_Rest
                beq Play_Keyoff
                endif

Play_FiltPos:   ldy #$00
                beq Play_FiltDone
                bmi Play_FiltInit
Play_FiltCutoff:lda #$00
Play_FiltLimitTblM1Access:
                cmp filtLimitTbl-1,y
                beq Play_FiltNext
                clc
Play_FiltSpdTblM1Access:
                adc filtSpdTbl-1,y
Play_StoreCutoff:
                sta Play_FiltCutoff+1
                sta $d416
Play_FiltDone:
Play_FiltType:  lda #$00
Play_MasterVol: ora #$0f                        ;Can be modified for fadein/out
                sta $d418
                jsr Play_ChnExec
                ldx #$07
                jsr Play_ChnExec
                ldx #$0e

                if PLAYER_SFX != 1
Play_ChnExec:   inc chnCounter,x
                bmi Play_PulseExec
                bne Play_Reload
                ldy chnSongPos,x
                else
Play_ChnExec:   ldy chnSongPos,x
                beq Play_JumpToSfx
                inc chnCounter,x
                bmi Play_PulseExec
                bne Play_Reload
                endif
Play_NewNotes:  lda (trackPtrLo),y
                tay
Play_PattTblLoM1Access:
                lda pattTblLo-1,y
                sta pattPtrLo
Play_PattTblHiM1Access:
                lda pattTblHi-1,y
                sta pattPtrHi
                ldy chnPattPos,x
                lda (pattPtrLo),y
                cmp #WAVEPTR
                bcs Play_Commands
                lsr
                bcs Play_NoNewIns
                adc chnTrans,x
                ora #$80
                sta chnNote,x
                iny
                lda (pattPtrLo),y
                sta chnIns,x
                bmi Play_Rest                   ;Instruments $81-$ff are $01-$7f as legato
Play_HardRestart:
                if PLAYER_SFX = 2
                lda chnSfxPtrHi,x
                bne Play_Rest
                endif
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
Play_PattEnd:   inc chnSongPos,x
                dc.b $24                        ;Skip next instruction
Play_NoPattEnd: tya
                sta chnPattPos,x
                if PLAYER_SFX = 2
                lda chnSfxPtrHi,x
                bne Play_JumpToSfx
                endif
                rts

Play_Reload:    lda chnDuration,x
                sta chnCounter,x
                lda chnPattPos,x
                beq Play_Sequencer
Play_NoSequencer:
                lda chnNote,x
                bpl Play_PulseExec
                jmp Play_NewNoteInit

                if PLAYER_SFX > 0
Play_JumpToSfx: jmp Play_SfxExec
                endif

Play_PulseExec: if PLAYER_SFX = 2
                lda chnSfxPtrHi,x
                bne Play_JumpToSfx
                endif
                ldy chnPulsePos,x
                beq Play_WaveExec
                bmi Play_PulseInit
                lda chnPulse,x
Play_PulseLimitTblM1Access:
                cmp pulseLimitTbl-1,y
                beq Play_PulseNext
                clc
Play_PulseSpdTblM1Access:
                adc pulseSpdTbl-1,y
                adc #$00
Play_StorePulse:
                sta chnPulse,x
                sta $d402,x
                sta $d403,x

Play_WaveExec:  ldy chnWavePos,x
                beq Play_WaveDone
Play_WaveTblM1Access:
                lda waveTbl-1,y
                beq Play_Vibrato
                cmp #$90
                bcs Play_SlideOrDelay
Play_SetWave:   sta chnWave,x
                sta $d404,x
Play_NoWaveChange:
Play_WaveNextTblM1Access1:
                lda waveNextTbl-1,y
                sta chnWavePos,x
Play_NoteTblM1Access1:
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

Play_Sequencer: ldy chnSongPos,x
                lda (trackPtrLo),y
                bmi Play_SongTrans
                bne Play_SequencerDone
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
                if PLAYER_SFX = 2
                lda chnSfxPtrHi,x
                bne Play_JumpToSfx
                endif
                lda chnNote,x
                bpl Play_WaveExec
                bmi Play_NewNoteInit

Play_PulseInit: 
Play_PulseNextTblM81Access:
                lda pulseNextTbl-$81,y
                sta chnPulsePos,x
Play_PulseLimitTblM81Access:
                lda pulseLimitTbl-$81,y
                jmp Play_StorePulse
Play_PulseNext:
Play_PulseNextTblM1Access:
                lda pulseNextTbl-1,y
                sta chnPulsePos,x
                jmp Play_WaveExec

Play_SlideOrDelay:
                beq Play_Slide
Play_WaveDelay: adc chnWaveTime,x
                bne Play_WaveDelayNotOver
                clc
                sta chnWaveTime,x
                bcc Play_NoWaveChange

Play_Vibrato:   lda chnWaveTime,x
                bpl Play_VibNoDir
Play_NoteTblM1Access2:
                cmp noteTbl-1,y
                bcs Play_VibNoDir2
                eor #$ff
Play_VibNoDir:  sec
Play_VibNoDir2: sbc #$02
                sta chnWaveTime,x
                lsr
                lda chnFreqLo,x
                bcs Play_VibDown
Play_VibUp:
Play_WaveNextTblM1Access2:
                adc waveNextTbl-1,y
                sta chnFreqLo,x
                sta $d400,x
                bcc Play_VibDone
                lda chnFreqHi,x
                adc #$00
                jmp Play_StoreFreqHi
Play_VibDown:
Play_WaveNextTblM1Access3:
                sbc waveNextTbl-1,y
                sta chnFreqLo,x
                sta $d400,x
                bcs Play_VibDone
                lda chnFreqHi,x
                sbc #$00
                jmp Play_StoreFreqHi

Play_LegatoNoteInit:
                tya
                and #$7f
                tay
                bpl Play_FinishLegatoInit

Play_Slide:     lda chnFreqLo,x
Play_NoteTblM1Access3:
                adc noteTbl-1,y                 ;Note: speed must be stored as speed-1 due to C=1 here
                sta chnFreqLo,x
                sta $d400,x
Play_WaveNextTblM1Access4:
                lda waveNextTbl-1,y
Play_SfxFreqMod:adc chnFreqHi,x
                jmp Play_StoreFreqHi

Play_NewNoteInit:
                and #$7f
                sta chnNote,x                   ;Reset newnote-flag
                ldy chnIns,x
                bmi Play_LegatoNoteInit
Play_HRNoteInit:
Play_InsADM1Access:
                lda insAD-1,y                   ;Instruments are 1-indexed just so that the converter can
                sta $d405,x                     ;differentiate between "no instrument change" and the first
Play_InsSRM1Access:                             ;instrument. Strictly speaking they wouldn't need to be
                lda insSR-1,y
                sta $d406,x
                lda #$09                        ;Fixed 1stframe wave
                sta $d404,x
Play_FinishLegatoInit:
Play_InsPulsePosM1Access:
                lda insPulsePos-1,y
                beq Play_SkipPulseInit
                sta chnPulsePos,x
Play_SkipPulseInit:
Play_InsFiltPosM1Access:
                lda insFiltPos-1,y
                beq Play_SkipFiltInit
                sta Play_FiltPos+1
Play_SkipFiltInit:
Play_InsWavePosM1Access:
                lda insWavePos-1,y
Play_SetWavePos:sta chnWavePos,x
                lda #$00
                sta chnWaveTime,x
Play_VibDone:   rts

                if PLAYER_SFX > 0

        ; Sound effect support code

Play_SfxDelayOngoing:
                inc chnSfxTime,x
Play_SfxEffects:lda chnSfxFreqMod,x
                asl
                bne Play_SfxFreqMod
                rts
Play_SfxEnd:    sta chnSfxPtrHi,x
                if PLAYER_SFX = 2
                sta chnWavePos,x                ;Also reset wavepos when returning from sound FX to music
                endif
Play_SfxDone:   rts

Play_SfxExec:   if PLAYER_SFX = 1
                lda chnSfxPtrHi,x
                beq Play_SfxDone
                endif
                sta pattPtrHi
                if PLAYER_SFX = 2
                lda chnNote,x                   ;Prevent newnoteinit triggering during sound FX
                bpl Play_SfxNoNewNote
                and #$7f
                sta chnNote,x
Play_SfxNoNewNote:
                endif
                lda chnSfxPtrLo,x
                sta pattPtrLo
                ldy chnSfxPos,x
                lda (pattPtrLo),y
                beq Play_SfxEnd
                bpl Play_NoSfxDelay
                sec
                adc chnSfxTime,x
                bne Play_SfxDelayOngoing
Play_SfxDelayDone:
                sta chnSfxTime,x
                inc chnSfxPos,x                 ;Delay ended, run still effects only this frame
                bne Play_SfxEffects
Play_NoSfxDelay:sta sfxTemp
Play_NoSfxEnd:  iny
                lsr sfxTemp
                bcc Play_NoSfxHR
                lda #$0f
                sta $d406,x
                lda chnWave,x
                and #$fe
                sta $d404,x
                lda #$00
                sta chnSfxFreqMod,x
                sta chnSfxTime,x
                beq Play_NoSfxFreqMod

Play_NoSfxHR:   lsr sfxTemp
                bcc Play_NoSfxFirstFrame
                lda (pattPtrLo),y
                sta $d405,x
                iny
                lda (pattPtrLo),y
                sta $d406,x
                iny
                lda #$09
                sta $d404,x

Play_NoSfxFirstFrame:
                lsr sfxTemp
                bcc Play_NoSfxPulse
                lda (pattPtrLo),y
                sta $d402,x
                sta $d403,x
                iny

Play_NoSfxPulse:
                lsr sfxTemp
                bcc Play_NoSfxWave
                lda (pattPtrLo),y
                sta chnWave,x
                sta $d404,x
                iny

Play_NoSfxWave: lsr sfxTemp
                bcc Play_NoSfxFreq
                lda (pattPtrLo),y
                sta chnFreqHi,x
                sta $d400,x
                sta $d401,x
                iny

Play_NoSfxFreq: lsr sfxTemp
                bcc Play_NoSfxFreqMod
                lda (pattPtrLo),y
                sta chnSfxFreqMod,x
                iny

Play_NoSfxFreqMod:
                tya
                sta chnSfxPos,x
                jmp Play_SfxEffects

                endif

Play_DoInit:    dex
                txa
                sta pattPtrLo
                asl
                asl
                adc pattPtrLo
                tay
Play_SongTblAccess1:
                lda songTbl,y
                sta trackPtrLo
                iny
Play_SongTblAccess2:
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
Play_ChnInit:
Play_SongTblAccess3:
                lda songTbl,y
                sta chnSongPos,x
                iny
                lda #$ff
                sta chnDuration,x
                lda #$00
                sta chnCounter,x
                sta chnNote,x
                sta chnPattPos,x
                sta chnWavePos,x
                sta chnPulsePos,x
                sta $d404,x
                rts

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

                if PLAYER_SFX = 2
chnSfxPos:      dc.b 0
chnSfxPtrLo:    dc.b 0
chnSfxPtrHi:    dc.b 0
                dc.b 0
                dc.b 0
                dc.b 0
                dc.b 0

                dc.b 0,0,0,0,0,0,0
                dc.b 0,0,0
                endif

                if PLAYER_SFX = 1
chnSfxPos       = chnCounter
chnSfxPtrLo     = chnNote
chnSfxPtrHi     = chnIns
                endif

                if PLAYER_SFX > 0
chnSfxTime      = chnWaveTime
chnSfxFreqMod   = chnWavePos
                endif
freqTbl:
                dc.w $022d,$024e,$0271,$0296,$02be,$02e8,$0314,$0343,$0374,$03a9,$03e1,$041c
                dc.w $045a,$049c,$04e2,$052d,$057c,$05cf,$0628,$0685,$06e8,$0752,$07c1,$0837
                dc.w $08b4,$0939,$09c5,$0a5a,$0af7,$0b9e,$0c4f,$0d0a,$0dd1,$0ea3,$0f82,$106e
                dc.w $1168,$1271,$138a,$14b3,$15ee,$173c,$189e,$1a15,$1ba2,$1d46,$1f04,$20dc
                dc.w $22d0,$24e2,$2714,$2967,$2bdd,$2e79,$313c,$3429,$3744,$3a8d,$3e08,$41b8
                dc.w $45a1,$49c5,$4e28,$52cd,$57ba,$5cf1,$6278,$6853,$6e87,$751a,$7c10,$8371
                dc.w $8b42,$9389,$9c4f,$a59b,$af74,$b9e2,$c4f0,$d0a6,$dd0e,$ea33,$f820,$ffff

                if PLAYER_MODULES > 0

fixupDestLoTbl: dc.b <Play_FiltNextTblM81Access
                dc.b <Play_FiltNextTblM1Access
                dc.b <Play_FiltSpdTblM81Access
                dc.b <Play_FiltSpdTblM1Access
                dc.b <Play_FiltLimitTblM81Access
                dc.b <Play_FiltLimitTblM1Access
                dc.b <Play_PulseNextTblM81Access
                dc.b <Play_PulseNextTblM1Access
                dc.b <Play_PulseSpdTblM1Access
                dc.b <Play_PulseLimitTblM81Access
                dc.b <Play_PulseLimitTblM1Access
                dc.b <Play_WaveNextTblM1Access4
                dc.b <Play_WaveNextTblM1Access3
                dc.b <Play_WaveNextTblM1Access2
                dc.b <Play_WaveNextTblM1Access1
                dc.b <Play_NoteTblM1Access3
                dc.b <Play_NoteTblM1Access2
                dc.b <Play_NoteTblM1Access1
                dc.b <Play_WaveTblM1Access
                dc.b <Play_InsFiltPosM1Access
                dc.b <Play_InsPulsePosM1Access
                dc.b <Play_InsWavePosM1Access
                dc.b <Play_InsSRM1Access
                dc.b <Play_InsADM1Access
                dc.b <Play_PattTblHiM1Access
                dc.b <Play_PattTblLoM1Access
                dc.b <Play_SongTblAccess3
                dc.b <Play_SongTblAccess2
                dc.b <Play_SongTblAccess1

fixupDestHiTbl: dc.b >Play_FiltNextTblM81Access
                dc.b >Play_FiltNextTblM1Access
                dc.b >Play_FiltSpdTblM81Access
                dc.b >Play_FiltSpdTblM1Access
                dc.b >Play_FiltLimitTblM81Access
                dc.b >Play_FiltLimitTblM1Access
                dc.b >Play_PulseNextTblM81Access
                dc.b >Play_PulseNextTblM1Access
                dc.b >Play_PulseSpdTblM1Access
                dc.b >Play_PulseLimitTblM81Access
                dc.b >Play_PulseLimitTblM1Access
                dc.b >Play_WaveNextTblM1Access4
                dc.b >Play_WaveNextTblM1Access3
                dc.b >Play_WaveNextTblM1Access2
                dc.b >Play_WaveNextTblM1Access1
                dc.b >Play_NoteTblM1Access3
                dc.b >Play_NoteTblM1Access2
                dc.b >Play_NoteTblM1Access1
                dc.b >Play_WaveTblM1Access
                dc.b >Play_InsFiltPosM1Access
                dc.b >Play_InsPulsePosM1Access
                dc.b >Play_InsWavePosM1Access
                dc.b >Play_InsSRM1Access
                dc.b >Play_InsADM1Access
                dc.b >Play_PattTblHiM1Access
                dc.b >Play_PattTblLoM1Access
                dc.b >Play_SongTblAccess3
                dc.b >Play_SongTblAccess2
                dc.b >Play_SongTblAccess1

fixupTypeTbl:   dc.b FIXUP_NOSIZE+FIXUP_MINUS81
                dc.b FIXUP_FILTSIZE+FIXUP_MINUS1
                dc.b FIXUP_NOSIZE+FIXUP_MINUS81
                dc.b FIXUP_FILTSIZE+FIXUP_MINUS1
                dc.b FIXUP_NOSIZE+FIXUP_MINUS81
                dc.b FIXUP_PULSESIZE+FIXUP_MINUS1
                dc.b FIXUP_NOSIZE+FIXUP_MINUS81
                dc.b FIXUP_PULSESIZE+FIXUP_MINUS1
                dc.b FIXUP_PULSESIZE+FIXUP_MINUS1
                dc.b FIXUP_NOSIZE+FIXUP_MINUS81
                dc.b FIXUP_WAVESIZE+FIXUP_MINUS1
                dc.b FIXUP_NOSIZE+FIXUP_MINUS1
                dc.b FIXUP_NOSIZE+FIXUP_MINUS1
                dc.b FIXUP_NOSIZE+FIXUP_MINUS1
                dc.b FIXUP_WAVESIZE+FIXUP_MINUS1
                dc.b FIXUP_NOSIZE+FIXUP_MINUS1
                dc.b FIXUP_NOSIZE+FIXUP_MINUS1
                dc.b FIXUP_WAVESIZE+FIXUP_MINUS1
                dc.b FIXUP_INSSIZE+FIXUP_MINUS1
                dc.b FIXUP_INSSIZE+FIXUP_MINUS1
                dc.b FIXUP_INSSIZE+FIXUP_MINUS1
                dc.b FIXUP_INSSIZE+FIXUP_MINUS1
                dc.b FIXUP_INSSIZE+FIXUP_MINUS1
                dc.b FIXUP_PATTSIZE+FIXUP_MINUS1
                dc.b FIXUP_PATTSIZE+FIXUP_MINUS1
                dc.b FIXUP_SONGSIZE+FIXUP_MINUS1
                dc.b FIXUP_NOSIZE
                dc.b FIXUP_NOSIZE
                dc.b FIXUP_NOSIZE

fixupSubTbl:    dc.b $00,$01,$81

        ; In settable music data mode, include dummy music data labels so that the player
        ; will not complain when assembled

musicHeader:
songTbl:
pattTblLo:
pattTblHi:
insAD:
insSR:
insWavePos:
insPulsePos:
insFiltPos:
waveTbl:
noteTbl:
waveNextTbl:
pulseLimitTbl:
pulseSpdTbl:
pulseNextTbl:
filtLimitTbl:
filtSpdTbl:
filtNextTbl:

                endif