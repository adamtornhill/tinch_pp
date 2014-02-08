set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})
include(Erlang)

option(BUILD_TEST "Build test applications" ON)
option(INSTALL_TEST "Install test applications" ON)

if(BUILD_TEST)
  SET(Boost_ADDITIONAL_VERSIONS "1.42" "1.42.0" "1.43" "1.43.0" "1.44" "1.45" "1.46" "1.46.1")

  find_package( Boost 1.42.0 
    COMPONENTS
      thread
      system
      regex
      signals
    REQUIRED )
 
  find_program( ERLANG_ERLC erlc
    DOC "Erlang compiler")

  link_directories(${Boost_LIBRARY_DIRS})

  add_erlang(reflect_msg chat_server link_tester)

  add_definitions(-DBOOST_ALL_NO_LIB)
  include_directories(${CMAKE_SOURCE_DIR} ${Boost_INCLUDE_DIRS})
 
  add_executable(net_kernel_sim net_kernel_sim.cpp)
  target_link_libraries(net_kernel_sim tinch++ ${Boost_LIBRARIES})

  add_executable(patterns patterns.cpp)
  target_link_libraries(patterns tinch++ ${Boost_LIBRARIES})
  
  add_executable(patterns_testing_any patterns_testing_any.cpp)
  target_link_libraries(patterns_testing_any tinch++ ${Boost_LIBRARIES})
  
  add_executable(patterns_testing_assign patterns_testing_assign.cpp)
  target_link_libraries(patterns_testing_assign tinch++ ${Boost_LIBRARIES})

  add_executable(rpc_test rpc_test.cpp)
  target_link_libraries(rpc_test tinch++ ${Boost_LIBRARIES})

  add_executable(thread_safe_queue thread_safe_queue.cpp)
  target_link_libraries(thread_safe_queue tinch++ ${Boost_LIBRARIES})
  
  add_executable(chat_client chat_client.cpp)
  target_link_libraries(chat_client tinch++ ${Boost_LIBRARIES})
  
  add_executable(local_link local_link.cpp)
  target_link_libraries(local_link tinch++ ${Boost_LIBRARIES})
  
  add_executable(remote_link remote_link.cpp)
  target_link_libraries(remote_link tinch++ ${Boost_LIBRARIES})
  
  add_executable(mbox_same_node_links mbox_same_node_links.cpp)
  target_link_libraries(mbox_same_node_links tinch++ ${Boost_LIBRARIES})

  add_test(thread_safe_queue_test thread_safe_queue)

  find_program(VALGRIND_EXE valgrind)
  if(VALGRIND_EXE)
    set(MEMCHECK_CMD ${VALGRIND_EXE} --tool=memcheck --leak-check=yes --show-reachable=yes --num-callers=20 --track-fds=yes)
    set(PROFILING_CMD ${VALGRIND_EXE} --tool=callgrind --dump-instr=yes --simulate-cache=yes --collect-jumps=yes)
    add_custom_target(memcheck)
    add_custom_target(profiling)

    add_custom_command(TARGET memcheck
      COMMAND ${MEMCHECK_CMD} ./thread_safe_queue
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

    add_custom_command(TARGET profiling
      COMMAND ${PROFILING_CMD} --callgrind-out-file=thread_safe_queue.callgrind ./thread_safe_queue
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
  endif(VALGRIND_EXE)

  if(INSTALL_TEST)
    install(TARGETS net_kernel_sim patterns rpc_test thread_safe_queue chat_client patterns_testing_any patterns_testing_assign local_link remote_link mbox_same_node_links
      DESTINATION ${CMAKE_PROJECT_NAME}-${CPACK_PACKAGE_VERSION}/test )

    if(ERLANG_OUTPUT_FILES)
      install(
	FILES 
	  ${ERLANG_OUTPUT_FILES}
	DESTINATION ${CMAKE_PROJECT_NAME}-${CPACK_PACKAGE_VERSION}/test )
    endif(ERLANG_OUTPUT_FILES)
  endif(INSTALL_TEST)
endif(BUILD_TEST)
