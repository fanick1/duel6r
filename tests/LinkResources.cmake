# Links the game resources next to a binary, so the game can run from its build directory.
# Usage: cmake -DSOURCE_DIR=<repository> -DBINARY_DIR=<directory of the binary> -P LinkResources.cmake
foreach (dir data levels profiles sound textures shaders)
    if (NOT EXISTS "${BINARY_DIR}/${dir}")
        file(CREATE_LINK "${SOURCE_DIR}/resources/${dir}" "${BINARY_DIR}/${dir}" SYMBOLIC)
    endif ()
endforeach ()
