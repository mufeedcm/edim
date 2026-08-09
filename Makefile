all: clean run-dev

build-dev: 
	mkdir -p build
	cmake -S . -B build/desktop -DUSE_VENDORED=OFF
	cmake --build build/desktop

run-dev: build-dev
	@if [ -d "build/desktop/edim.app" ]; then \
		open ./build/desktop/edim.app; \
	else \
	./build/desktop/edim; \
	fi

package: 
	mkdir -p build
	cmake -S . -B build/release -DUSE_VENDORED=ON
	cmake --build build/release
	cd build/release && cpack

build-web:
	mkdir -p build
	source ~/emsdk/emsdk_env.sh
	emcmake cmake -S . -B build/web
	cmake --build build/web

run-web: build-web
	emrun build/web/edim.html

clean: 
	rm -rf build 

.PHONY: all build-dev run-dev package build-web run-web clean
