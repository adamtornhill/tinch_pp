SET(Boost_ADDITIONAL_VERSIONS "1.42" "1.42.0" "1.43" "1.43.0" "1.44" "1.45" "1.46" "1.46.1" )
find_package( Boost 1.42.0 REQUIRED )

SET(CPP_SOURCE
  actual_mailbox.cpp
  actual_node.cpp
  constants.cpp
  control_msgs_impl.cpp
  ctrl_msg_dispatcher.cpp
  epmd_requestor.cpp
  erlang_value_types.cpp
  erl_any.cpp
  erl_string.cpp
  exceptions.cpp
  ext_message_builder.cpp
  link_policies.cpp
  linker.cpp
  matchable_range.cpp
  matchable_seq.cpp
  md5.cpp
  node_async_tcp_ip.cpp
  node_connection.cpp
  node_connection_state.cpp
  node_connector.cpp
  node.cpp
  rpc.cpp
  type_makers.cpp
  types.cpp
  utils.cpp
)

include_directories(${CMAKE_SOURCE_DIR} ${Boost_INCLUDE_DIRS})
add_library(tinch++ ${CPP_SOURCE})

install(TARGETS tinch++ 
  DESTINATION ${CMAKE_PROJECT_NAME}-${CPACK_PACKAGE_VERSION}/lib )

file(GLOB HEADER_FILES *.h)
install(FILES 
    ${HEADER_FILES}
  DESTINATION ${CMAKE_PROJECT_NAME}-${CPACK_PACKAGE_VERSION}/impl
  )
