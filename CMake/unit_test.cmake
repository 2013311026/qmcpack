# Runs unit tests
FUNCTION( ADD_UNIT_TEST TESTNAME TEST_BINARY )
    MESSAGE("Adding test ${TESTNAME}")
    IF ( USE_MPI )
        ADD_TEST(NAME ${TESTNAME} COMMAND ${MPIEXEC} ${MPIEXEC_NUMPROC_FLAG} 1 ${TEST_BINARY})
        #SET_TESTS_PROPERTIES( ${TESTNAME} PROPERTIES ENVIRONMENT OMP_NUM_THREADS=1 )
    ELSE()
        ADD_TEST(NAME ${TESTNAME} COMMAND ${TEST_BINARY})
        #SET_TESTS_PROPERTIES( ${TESTNAME} PROPERTIES ENVIRONMENT OMP_NUM_THREADS=1 )
    ENDIF()
    SET_PROPERTY(TEST ${TESTNAME} APPEND PROPERTY LABELS unit)

    SET(TEST_LABELS "")
    EXECUTE_PROCESS(COMMAND ${PROJECT_SOURCE_DIR}/tests/scripts/test_labels.py ${TESTNAME} ${QMC_CUDA} ${ENABLE_SOA} ${QMC_COMPLEX} ${QMC_MIXED_PRECISION} OUTPUT_VARIABLE TEST_LABELS RESULT_VARIABLE SUCCESS)
    IF( NOT ${SUCCESS} STREQUAL "0" )
      MESSAGE("Test labeling failed.  Test labeling error output:\n${TEST_LABELS}")
      SET(TEST_LABELS "")
    ENDIF()
    SET_PROPERTY(TEST ${TESTNAME} APPEND PROPERTY LABELS ${TEST_LABELS} )

ENDFUNCTION()
