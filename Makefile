.PHONY: build
.PHONY: build32
.PHONY: build64
.PHONY: clean

build: build32 build64
	@rm -f temp.c

gentemp:
	@sed "s/0000000000/$$(($$(date +\%s)+172800))/" processdumper.c > temp.c

build64: gentemp
	@x86_64-w64-mingw32-gcc -o pdump.exe temp.c -ldbghelp

build32: gentemp
	@i686-w64-mingw32-gcc -m32 -o pdump32.exe temp.c -ldbghelp

clean:
	@rm -f pdump.exe pdump32.exe
	@rm -f temp.c
