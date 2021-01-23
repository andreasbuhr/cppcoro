add_library(doctest::doctest INTERFACE IMPORTED)
target_include_directories(doctest::doctest INTERFACE doctest)

include(${CMAKE_CURRENT_LIST_DIR}/doctest/doctest.cmake)

find_package(Threads REQUIRED)

add_library(tests-main STATIC
	main.cpp
	counted.cpp
)
target_link_libraries(tests-main PUBLIC cppcoro doctest::doctest Threads::Threads)

set(tests
	generator_tests.cpp
	recursive_generator_tests.cpp
	async_generator_tests.cpp
	async_auto_reset_event_tests.cpp
	async_manual_reset_event_tests.cpp
	async_mutex_tests.cpp
	async_latch_tests.cpp
	cancellation_token_tests.cpp
	task_tests.cpp
	sequence_barrier_tests.cpp
	shared_task_tests.cpp
	sync_wait_tests.cpp
	single_consumer_async_auto_reset_event_tests.cpp
	single_producer_sequencer_tests.cpp
	multi_producer_sequencer_tests.cpp
	when_all_tests.cpp
	when_all_ready_tests.cpp
	ip_address_tests.cpp
	ip_endpoint_tests.cpp
	ipv4_address_tests.cpp
	ipv4_endpoint_tests.cpp
	ipv6_address_tests.cpp
	ipv6_endpoint_tests.cpp
	static_thread_pool_tests.cpp
)

if(WIN32)
    list(APPEND tests
        scheduling_operator_tests.cpp
        io_service_tests.cpp
        file_tests.cpp
        socket_tests.cpp
    )
else()
	# let more time for some tests
	set(async_auto_reset_event_tests_TIMEOUT 60)
endif()

foreach(test ${tests})
	get_filename_component(test_name ${test} NAME_WE)
	add_executable(${test_name} ${test})
	target_link_libraries(${test_name} PRIVATE tests-main)
	string(REPLACE "_" " " test_prefix ${test_name})
	if (NOT DEFINED ${test_name}_TIMEOUT)
		set(${test_name}_TIMEOUT 30)
	endif()
	doctest_discover_tests(${test_name} TEST_PREFIX ${test_prefix}- PROPERTIES TIMEOUT ${${test_name}_TIMEOUT})
endforeach()

function(add_usage_test variant_name cppcoro_ROOT)
	set(APP_BINARY_DIR ${CPPCORO_USAGE_TEST_DIR}/app_build/${variant_name})
	add_test(
		NAME app_configure_${variant_name}
		COMMAND
			${CMAKE_COMMAND} 
				-S ${CMAKE_CURRENT_LIST_DIR}/use_cppcoro
				-B ${APP_BINARY_DIR}
				-G ${CMAKE_GENERATOR}
				-D CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
				-D CMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
				-D CMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}
				-D cppcoro_ROOT=${cppcoro_ROOT}
	)
	add_test(
		NAME app_build_${variant_name}
		COMMAND
			${CMAKE_COMMAND}
				--build ${APP_BINARY_DIR}
				--config ${CMAKE_BUILD_TYPE}
	)
	set_tests_properties(
		app_configure_${variant_name}
		PROPERTIES
			FIXTURES_SETUP app_build_${variant_name}_requires
			TIMEOUT 30
	)
	set_tests_properties(
		app_build_${variant_name}
		PROPERTIES
			FIXTURES_REQUIRED app_build_${variant_name}_requires
			TIMEOUT 30
	)
endfunction()

if(CPPCORO_USAGE_TEST_DIR)
	if(NOT IS_ABSOLUTE ${CPPCORO_USAGE_TEST_DIR})
		set(CPPCORO_USAGE_TEST_DIR ${PROJECT_BINARY_DIR}/${CPPCORO_USAGE_TEST_DIR})
	endif()

	add_usage_test(with_cppcoro_build_tree ${PROJECT_BINARY_DIR})

	if (CMAKE_VERSION VERSION_GREATER_EQUAL 3.15)
		set(CPPCORO_USAGE_TEST_INSTALL_DIR ${CPPCORO_USAGE_TEST_DIR}/cppcoro_install)
	
		add_usage_test(with_cppcoro_install_dir ${CPPCORO_USAGE_TEST_INSTALL_DIR})
	
		add_test(
			NAME cppcoro_usage_test_install
			COMMAND
				${CMAKE_COMMAND}
					--install ${PROJECT_BINARY_DIR}
					--config ${CMAKE_BUILD_TYPE}
					--prefix ${CPPCORO_USAGE_TEST_INSTALL_DIR}
		)
		set_tests_properties(
			cppcoro_usage_test_install
			PROPERTIES
				FIXTURES_SETUP app_configure_with_cppcoro_install_dir_requires
				TIMEOUT 30
		)
		set_tests_properties(
			app_configure_with_cppcoro_install_dir
			PROPERTIES
				FIXTURES_REQUIRED app_configure_with_cppcoro_install_dir_requires
		)
	endif()
endif()
