all: gt2mini example.prg example.sid

clean:
	rm -f gt2mini
	rm -f *.prg
	rm -f *.sid

gt2mini: gt2mini.c fileio.c
	gcc gt2mini.c fileio.c -o gt2mini

example.prg: gt2mini mw4title.sng prgexample.s player.s
	./gt2mini mw4title.sng musicmodule.s -s1000
	dasm musicmodule.s -omusicmodule.bin -p3 -f3
	dasm prgexample.s -oexample.prg -p3

example.sid: gt2mini mw4title.sng sidexample.s player.s
	./gt2mini mw4title.sng musicdata.s -b
	dasm sidexample.s -oexample.sid -p3 -f3