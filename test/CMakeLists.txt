#Boost.Test uses typeid for no reason
if(NOT SILICIUM_NO_RTTI)
	include_directories(.)
	file(GLOB sources "*.hpp" "*.cpp" "../ventura_tests/*.cpp")
	file(GLOB_RECURSE headers "../ventura/*.hpp")
	add_executable(unit_test ${sources} ${headers})
	target_link_libraries(unit_test ${Boost_LIBRARIES})
endif()
