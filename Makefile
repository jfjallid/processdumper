.PHONY: build
.PHONY: build64
.PHONY: clean

build: build64
	@rm -f temp.c

gentemp:
	@sed "s/0000000000/$$(($$(date +\%s)+172800))/" processdumper.c > temp.c

build64: gentemp
	@x86_64-w64-mingw32-gcc -o pdump.exe temp.c -ldbghelp

clean:
	@rm -f pdump.exe
	@rm -f temp.c
