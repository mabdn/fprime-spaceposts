set(CMAKE_CXX_STANDARD 17)
set(SOURCE_FILES
    "${CMAKE_CURRENT_LIST_DIR}/MessageStorage.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/MessageStorage.fpp"  
)
register_fprime_module()

# Register the unit test build
set(UT_SOURCE_FILES
    "${CMAKE_CURRENT_LIST_DIR}/MessageStorage.fpp"        
	"${CMAKE_CURRENT_LIST_DIR}/test/ut/main.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/Tester.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/model/SpacePostFile.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/model/StorageDirectorySetup.cpp"
)
set(UT_MOD_DEPS STest Os_Stubs) # Os_Stubs needed in UT-STO-110
set(UT_AUTO_HELPERS ON)
register_fprime_ut()