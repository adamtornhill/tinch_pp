install(FILES 
    erlang_types.h
    erlang_value_types.h
    erl_any.h
    erl_list.h
    erl_object.h
    erl_string.h  
    erl_tuple.h
    exceptions.h
    mailbox.h
    make_erl_tuple.h 
    matchable.h
    node.h
    rpc.h
    type_makers.h
  DESTINATION ${CMAKE_PROJECT_NAME}-${CPACK_PACKAGE_VERSION}/tinch_pp
  )
