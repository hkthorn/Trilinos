TRIBITS_SUBPACKAGE(Core)

TRIBITS_ADD_SHOW_DEPRECATED_WARNINGS_OPTION()

ADD_SUBDIRECTORY(src)

TRIBITS_ADD_TEST_DIRECTORIES(test)
#TRIBITS_ADD_EXAMPLE_DIRECTORIES(example)

TRIBITS_SUBPACKAGE_POSTPROCESS()

