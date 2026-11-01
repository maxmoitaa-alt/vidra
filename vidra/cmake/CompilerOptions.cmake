add_compile_options(-Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers)
if(CMAKE_BUILD_TYPE STREQUAL "Release")
  add_compile_options(-O2)
endif()
