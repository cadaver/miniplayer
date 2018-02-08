all: gt2mini.exe example.prg example.sid

clean:
	del *.exe

gt2mini.exe: gt2mini.c fileio.c
	gcc gt2mini.c fileio.c -ogt2mini.exe

example.prg: gt2mini.exe mw4title.sng prgexample.s player.s
	gt2mini mw4title.sng musicmodule.s -s1000
	dasm musicmodule.s -omusicmodule.bin -p3 -f3
	dasm prgexample.s -oexample.prg -p3

example.sid: gt2mini.exe mw4title.sng sidexample.s player.s
	gt2mini mw4title.sng musicdata.s -b
	dasm sidexample.s -oexample.sid -p3 -f3
