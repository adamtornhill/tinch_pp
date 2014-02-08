macro(add_erlang)
  set(ERLANG_SOURCES ${ARGV})

  if(ERLANG_ERLC)
    get_target_property(BUILD_ERLANG_TARGET build_erlang TYPE)
    if(NOT BUILD_ERLANG_TARGET)
      add_custom_target(build_erlang ALL)
    endif()

    foreach(ERLANG_SOURCE ${ERLANG_SOURCES})
      add_custom_target(${ERLANG_SOURCE} 
	COMMAND ${ERLANG_ERLC} -o ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/${ERLANG_SOURCE}.erl)
      add_dependencies(build_erlang ${ERLANG_SOURCE})
      set(ERLANG_OUTPUT_FILES ${ERLANG_OUTPUT_FILES} ${CMAKE_CURRENT_BINARY_DIR}/${ERLANG_SOURCE}.beam)
      set_property(DIRECTORY APPEND PROPERTY
		   ADDITIONAL_MAKE_CLEAN_FILES
		   "${ERLANG_SOURCE}.beam")
    endforeach(ERLANG_SOURCE ${ERLANG_SOURCES})
  else(ERLANG_ERLC)
    message("Erlang compiler not found")
  endif(ERLANG_ERLC)
endmacro(add_erlang)

