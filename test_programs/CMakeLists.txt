add_executable(cat "cat.cpp")
target_link_libraries(cat ${CONAN_LIBS} ${Boost_LIBRARIES} ${VENTURA_EXTRA_LIBS})

file(GLOB directory "*.cpp" "*.hpp")
set(formatted ${formatted} ${directory} PARENT_SCOPE)
