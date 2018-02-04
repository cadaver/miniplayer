all: gt2mini.exe player.prg player.sid

clean:
	del *.exe

gt2mini.exe: gt2mini.c fileio.c
	gcc gt2mini.c fileio.c -ogt2mini.exe

musicdata.s: mw4title.sng
	gt2mini mw4title.sng musicdata.s

player.prg: basicheader.s player.s musicdata.s
	dasm basicheader.s -oplayer.prg -p3

player.sid: sidheader.s player.s musicdata.s
	dasm sidheader.s -oplayer.sid -p3 -f3
