if not exist build (
	mkdir build
	pushd build
	cmake ..
	popd
)

pushd build
cmake --build . --target toy_iso_paintball
popd