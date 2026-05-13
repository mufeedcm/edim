build: 
	mkdir -p build
	cmake -S . -B build/macos
	cmake --build build/macos

run: build
	./build/macos/edim

build-web:
	mkdir -p build
	source ~/emsdk/emsdk_env.sh
	emcmake cmake -S . -B build/web
	cmake --build build/web

run-web: build-web
	emrun build/web/edim.html

clean: 
	rm -rf build 

