.PHONY: build
.PHONY: build64
.PHONY: clean

all: build64 buildsvc64

build: build64
	@rm -f temp.c

service: buildsvc64
	@rm -f temp.c

gentemp:
	@sed "s/0000000000/$$(($$(date +\%s)+172800))/" main.c > temp.c

gentempsvc:
	@sed "s/0000000000/$$(($$(date +\%s)+172800))/" svcmain.c > temp.c

build64: gentemp
	@x86_64-w64-mingw32-gcc -o pdump.exe temp.c processdumper.c -ldbghelp

buildsvc64: gentempsvc
	@x86_64-w64-mingw32-gcc -o pdumpsvc.exe temp.c processdumper.c -ldbghelp

clean:
	@rm -f pdump.exe
	@rm -f pdumpsvc.exe
	@rm -f temp.c
