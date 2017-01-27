if(UNIX) #TODO: Windows
	add_executable(run_process "run_process.cpp")
	target_link_libraries(run_process ${CONAN_LIBS} ${Boost_LIBRARIES} ${VENTURA_EXTRA_LIBS})
	set_target_properties(run_process PROPERTIES FOLDER examples)
endif()

file(GLOB exampleSources "*.cpp" "*.hpp")
set(formatted ${formatted} ${exampleSources} PARENT_SCOPE)
