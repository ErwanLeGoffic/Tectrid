CC=em++
SOURCES:=$(wildcard *.cpp) $(wildcard */*.cpp)
		
#FLAGS_DEBUG=-g4 -s PRECISE_F32=1 -s TOTAL_MEMORY=369098752

#FLAGS_RELEASE_ASMJS=-O3 --memory-init-file 0 -s PRECISE_F32=1 -s TOTAL_MEMORY=369098752 -s ALLOW_MEMORY_GROWTH=0 --memoryprofiler
FLAGS_RELEASE_ASMJS=-O3 --memory-init-file 0 -s PRECISE_F32=1 -s TOTAL_MEMORY=369098752 -s ALLOW_MEMORY_GROWTH=0
OUTPUT_ASMJS=cpplib

#FLAGS_RELEASE_WASM=-O3 --memory-init-file 0 -s PRECISE_F32=1 -s ALLOW_MEMORY_GROWTH=1 -s WASM=1 --memoryprofiler
FLAGS_RELEASE_WASM=-O3 --memory-init-file 0 -s PRECISE_F32=1 -s ALLOW_MEMORY_GROWTH=1 -s WASM=1
OUTPUT_WASM=cpplib_wasm

asmjs_only: $(SOURCES) $(OUTPUT_ASMJS)

wasm_only: $(SOURCES) $(OUTPUT_WASM)

all: $(SOURCES) $(OUTPUT_ASMJS) $(OUTPUT_WASM)

$(OUTPUT_ASMJS): $(SOURCES) 
	$(CC) $(SOURCES) --bind -std=c++11 -I . $(FLAGS_RELEASE_ASMJS) -o $(OUTPUT_ASMJS).js
	copy $(OUTPUT_ASMJS).* ..\..\WebBabylonApp\src
	copy $(OUTPUT_ASMJS).* ..\..\WebBabylonAppDood\src

$(OUTPUT_WASM): $(SOURCES)
	$(CC) $(SOURCES) --bind -std=c++11 -I . $(FLAGS_RELEASE_WASM) -o $(OUTPUT_WASM).js
	copy $(OUTPUT_WASM).* ..\..\WebBabylonApp\src
	copy $(OUTPUT_WASM).* ..\..\WebBabylonAppDood\src

clean:
	del $(OUTPUT_ASMJS).*
	del $(OUTPUT_WASM).*

