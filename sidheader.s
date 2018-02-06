                processor 6502
                org $0000

SUBTUNES        = 1

                dc.b "PSID"
                dc.b 0,2
                dc.b 0,$7c
                dc.b $00,$00
                dc.b $10,$00
                dc.b $10,$03
                dc.b 0,SUBTUNES
                dc.b 0,1
                dc.b 0,0,0,0

                org $0016
                dc.b "Minimal player test"

                org $0036
                dc.b "Cadaver"

                org $0056

                dc.b "2018 Covert Bitops"

                org $0076
                dc.b $00,$10

                org $007c
                dc.b $00,$10
                rorg $1000

                jmp Init

        ; Player configuration

PLAYER_ZPBASE   = $fc
PLAYER_SFX      = 0

        ; Player + musicdata

                include player.s

Init:           clc
                adc #$01
                sta PlayRoutine+1
                rts

                include musicdata.s
