

#
# A) Package-specific configuration options
#

#SET(HAVE_BOOL ON)
#SET(HAVE_STL ON)

TRIBITS_CONFIGURE_FILE(${PACKAGE_NAME}_config.h)

#
# B) Define the header and source files (and directories)
#

#
# src
#

SET(HEADERS "")
SET(SOURCES "")

TRIBITS_INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})
TRIBITS_INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/../headers)

SET(HEADERS ${HEADERS}
  ${CMAKE_CURRENT_BINARY_DIR}/${PACKAGE_NAME}_config.h
  )

#
# src
#

TRIBITS_INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

SET(HEADERS ${HEADERS}
  ../headers/Galeri_ConfigDefs.h
  ../headers/Galeri_Exception.h
  ../headers/Galeri_Version.h
  Galeri_Maps.h
  Galeri_CrsMatrices.h
  Galeri_VbrMatrices.h
  Galeri_FiniteElements.h
  Galeri_Utils.h
  )

SET(SOURCES ${SOURCES}
  Galeri_Maps.cpp
  Galeri_CrsMatrices.cpp
  Galeri_VbrMatrices.cpp
  Galeri_Utils.cpp
  )

IF (${PACKAGE_NAME}_ENABLE_Triutils)
  SET(HEADERS ${HEADERS}
    Galeri_ReadHB.h
    )
  SET(SOURCES ${SOURCES}
    Galeri_ReadHB.cpp
    )
ENDIF()

#
# src/Maps
#

TRIBITS_INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/Maps)

SET(HEADERS ${HEADERS}
  Maps/Galeri_Linear.h
  Maps/Galeri_Interlaced.h
  Maps/Galeri_Random.h
  Maps/Galeri_Cartesian2D.h
  Maps/Galeri_Cartesian3D.h
  Maps/Galeri_NodeCartesian2D.h
  )

#
# src/CrsMatrices
#

TRIBITS_INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/CRSMatrices)

SET(HEADERS ${HEADERS}
  CrsMatrices/Galeri_BigCross2D.h
  CrsMatrices/Galeri_BigStar2D.h
  CrsMatrices/Galeri_Cauchy.h
  CrsMatrices/Galeri_KMS.h
  CrsMatrices/Galeri_Ris.h
  CrsMatrices/Galeri_Cross2D.h
  CrsMatrices/Galeri_Laplace1DNeumann.h
  CrsMatrices/Galeri_Star2D.h
  CrsMatrices/Galeri_Cross3D.h
  CrsMatrices/Galeri_Lehmer.h
  CrsMatrices/Galeri_Stretched2D.h
  CrsMatrices/Galeri_Diag.h
  CrsMatrices/Galeri_Minij.h
  CrsMatrices/Galeri_Tridiag.h
  CrsMatrices/Galeri_Fielder.h
  CrsMatrices/Galeri_Ones.h
  CrsMatrices/Galeri_UniFlow2D.h
  CrsMatrices/Galeri_Hanowa.h
  CrsMatrices/Galeri_Parter.h
  CrsMatrices/Galeri_Vander.h
  CrsMatrices/Galeri_Hilbert.h
  CrsMatrices/Galeri_Pei.h
  CrsMatrices/Galeri_JordanBlock.h
  CrsMatrices/Galeri_Recirc2D.h
  CrsMatrices/Galeri_BentPipe2D.h
  )

#
# src/FiniteElements
#

TRIBITS_INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/FiniteElements)

SET(HEADERS ${HEADERS}
  FiniteElements/Galeri_AbstractGrid.h
  FiniteElements/Galeri_AbstractProblem.h
  FiniteElements/Galeri_AbstractQuadrature.h
  FiniteElements/Galeri_AbstractVariational.h
  FiniteElements/Galeri_FileGrid.h
  FiniteElements/Galeri_TriangleRectangleGrid.h
  FiniteElements/Galeri_QuadRectangleGrid.h
  FiniteElements/Galeri_HexCubeGrid.h
  FiniteElements/Galeri_TetCubeGrid.h
  FiniteElements/Galeri_TRIANGLEGrid.h
  FiniteElements/Galeri_QuadQuadrature.h
  FiniteElements/Galeri_TetQuadrature.h
  FiniteElements/Galeri_TriangleQuadrature.h
  FiniteElements/Galeri_HexQuadrature.h
  FiniteElements/Galeri_LinearProblem.h
  FiniteElements/Galeri_GalerkinVariational.h
  FiniteElements/Galeri_SUPGVariational.h
  FiniteElements/Galeri_MEDITInterface.h
  FiniteElements/Galeri_Workspace.h
  )

IF (TPL_ENABLE_MPI)

  SET(SOURCES ${SOURCES}

    )

ENDIF()

#
# C) Define the targets for package's library(s)
#

TRIBITS_ADD_LIBRARY(
  galeri-epetra
  HEADERS ${HEADERS}
  SOURCES ${SOURCES}
  )
