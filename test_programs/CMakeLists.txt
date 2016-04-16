add_executable(cat "cat.cpp")
target_link_libraries(cat ${Boost_LIBRARIES})

file(GLOB directory "*.cpp" "*.hpp")
set(formatted ${formatted} ${directory} PARENT_SCOPE)
