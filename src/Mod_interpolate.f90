! Brief  : A Module that contains multiple utility functions/types
! Author : César Godinho
! Date   : 13/08/2023


MODULE MOD_INTERPOLATE
    IMPLICIT NONE

    PUBLIC :: SplineData_t, GlobalSplineMap, INTERPOLATE_FROM_FILE, EVALUATE_SPLINE_DATA
    PRIVATE
    
    TYPE :: SplineData_t
        REAL(8), DIMENSION(:), POINTER     :: t ! Knots x val
        REAL(8), DIMENSION(:), POINTER     :: c ! Coeficients
        INTEGER                            :: n ! Dim of c and t
        INTEGER                            :: k ! Spline dimension
        INTEGER                            :: e ! Out of bounds evaluation strategy
    END TYPE SplineData_t

    TYPE :: SplineMapPair_t
        CHARACTER(128)                 :: key   = ''
        TYPE(SplineData_t)             :: value
        TYPE(SplineMapPair_t), POINTER :: next  => null()
        LOGICAL                        :: valid = .FALSE.
    END TYPE SplineMapPair_t

    TYPE :: SplineMap_t
        ! PRIVATE
        TYPE(SplineMapPair_t), ALLOCATABLE :: pairs(:)
        INTEGER                            :: capacity = 0
        INTEGER                            :: length   = 0

        CONTAINS
        PROCEDURE, PUBLIC :: init   => SPLINE_MAP_INIT
        PROCEDURE, PUBLIC :: free   => SPLINE_MAP_FREE
        PROCEDURE, PUBLIC :: find   => SPLINE_MAP_FIND
        PROCEDURE, PUBLIC :: insert => SPLINE_MAP_INSERT
    END TYPE SplineMap_t

    TYPE(SplineMap_t), TARGET :: GlobalSplineMap

    CONTAINS

    SUBROUTINE EVALUATE_SPLINE_DATA(spline_data, x, y)
        TYPE(SplineData_t), INTENT(IN)  :: spline_data
        REAL(8)           , INTENT(IN)  :: x
        REAL(8)           , INTENT(OUT) :: y
        INTEGER                         :: ierr

        CALL SPLEV(spline_data%t, spline_data%n, spline_data%c, spline_data%k, x, y, 1, spline_data%e, ierr)
        IF(ierr.GT.0) THEN
            WRITE(*,*) '------------------------------------------------------------------------------------------------------------------'
            WRITE(*,*) '       ERROR:           Spline evaluation failed this iteration.'
            WRITE(*,*) '------------------------------------------------------------------------------------------------------------------'
        ENDIF
    END SUBROUTINE

    SUBROUTINE INTERPOLATE_FROM_FILE(interpolator_file, output, s)
        CHARACTER(*)      , INTENT(IN)  :: interpolator_file
        TYPE(SplineData_t), INTENT(OUT) :: output
        REAL(8)           , INTENT(IN)  :: s
        
        INTEGER, PARAMETER :: iopt      = 0        ! Using a smooth spline (automatically calculated)
        INTEGER, PARAMETER :: k         = 3        ! B-spline type (cubic)

        INTEGER            :: lwrk      = 0        ! Size of the working space array
        INTEGER            :: nest      = 0        ! Max storage for knots (theoretically should be much higher)
        REAL(8)            :: sum       = 0.       ! Total sum of the points (y-wise)
        LOGICAL            :: file_ok              ! Interpolator function file exists

        ! Input/Output variables
        REAL(8), ALLOCATABLE         :: x(:), y(:), w(:)
        REAL(8), ALLOCATABLE, TARGET :: t(:), c(:)
        REAL(8), ALLOCATABLE         :: wrk(:)
        INTEGER, ALLOCATABLE         :: iwrk(:)
        INTEGER                      :: m, i, n
        REAL(8)                      :: r
        INTEGER                      :: ierr
        
        INQUIRE(FILE=TRIM(interpolator_file), EXIST=file_ok)
        IF(.NOT.file_ok) THEN
            WRITE(*,*) '------------------------------------------------------------------------------------------------------------------'
            WRITE(*,*) '       ERROR:           The specified interpolation file (', TRIM(interpolator_file), ') was not found.'
            WRITE(*,*) '       ERROR:           Aborting Execution...'
            WRITE(*,*) '------------------------------------------------------------------------------------------------------------------'
            ! TODO(César) : How do we handle this if we are inside an OpenMPI context???
! #ifdef OPENMPI_ON
!             CALL MPI_Abort(MPI_COMM_WORLD, 1, mpi_ierror)
! #endif
            STOP
        ENDIF

        ! Figure out the file size (i.e. the number of points)
        ! NOTE(César) : We can avoid a 2nd pass on the file if we use a vector like array aproach similar to the one on argparse.f90
        m = 0
        OPEN(1, FILE=TRIM(interpolator_file), STATUS='old')
        DO
            READ(1,*,END=10)
            m = m + 1
        END DO
        10 CLOSE(1)

        IF(m.LE.k) THEN
            WRITE(*,*) '------------------------------------------------------------------------------------------------------------------'
            WRITE(*,*) '       ERROR:           The specified interpolation file (', TRIM(interpolator_file), ') needs at least', k, 'points.'
            WRITE(*,*) '       ERROR:           Aborting Execution...'
            WRITE(*,*) '------------------------------------------------------------------------------------------------------------------'
            ! TODO(César) : How do we handle this if we are inside an OpenMPI context???
! #ifdef OPENMPI_ON
!             CALL MPI_Abort(MPI_COMM_WORLD, 1, mpi_ierror)
! #endif
            STOP
        ENDIF

        r    = 0
        nest = m+k+1
        n = m - 1
        lwrk = (m*(k+1)+nest*(7+3*k))*2
        WRITE(*,*) nest, lwrk, n
        ALLOCATE(wrk(lwrk), iwrk(nest))
        ALLOCATE(x(m), y(m), w(m))
        ALLOCATE(t(nest), c(nest))

        t = 0.
        c = 0.

        OPEN(1, file=TRIM(interpolator_file), status='old')
        DO i=1, m
            READ(1,*) x(i), y(i)
            sum = sum + y(i)
        END DO
        CLOSE(1)

        ! Normalize function and calculate the weights
        DO i=1, m
            IF (y(i).GT.0) THEN
                w(i) = 1/(DSQRT(y(i))) ! NOTE(César) : This assumes the spline data are a countlike simulation/set
            ELSE
                WRITE(*,*) '------------------------------------------------------------------------------------------------------------------'
                WRITE(*,*) '       ERROR:           The specified interpolation file (', TRIM(interpolator_file), ') requires positive y data.'
                WRITE(*,*) '       ERROR:           Aborting Execution...'
                WRITE(*,*) '------------------------------------------------------------------------------------------------------------------'
                ! TODO(César) : How do we handle this if we are inside an OpenMPI context???
    ! #ifdef OPENMPI_ON
    !             CALL MPI_Abort(MPI_COMM_WORLD, 1, mpi_ierror)
    ! #endif
                STOP
            ENDIF
        ENDDO
        
        CALL CURFIT(iopt, m, x, y, w, x(1), x(m), k, s, nest, n, t, c, r, wrk, lwrk, iwrk, ierr)

        IF(ierr.GT.0) THEN
            WRITE(*,*)           '------------------------------------------------------------------------------------------------------------------'
            WRITE(*,*)           '       ERROR:           Failed to calculate b-spline coefficients and/or knots.'
            WRITE(*,'(a,I2,a)') '        ERROR:           (ierr=', ierr, ')'
            WRITE(*,*)           '       ERROR:           Aborting Execution...'
            WRITE(*,*)           '------------------------------------------------------------------------------------------------------------------'
            ! TODO(César) : How do we handle this if we are inside an OpenMPI context???
! #ifdef OPENMPI_ON
!             CALL MPI_Abort(MPI_COMM_WORLD, 1, mpi_ierror)
! #endif
            STOP
        ENDIF

        output = SplineData_t(t, c, n, k, 1)

        DEALLOCATE(wrk, iwrk)
        DEALLOCATE(x, y, w)
    END SUBROUTINE

    SUBROUTINE INIT_GLOBAL_SPLINE_MAP(capacity) BIND(c, name='INIT_GLOBAL_SPLINE_MAP')
        USE, INTRINSIC :: ISO_C_BINDING ! shut the compiler up -> We just need the name='...' here
        INTEGER(c_int), INTENT(IN) :: capacity

        CALL GlobalSplineMap%init(capacity)
    END SUBROUTINE

    SUBROUTINE FREE_GLOBAL_SPLINE_MAP() BIND(c, name='FREE_GLOBAL_SPLINE_MAP')
        CALL GlobalSplineMap%free()
    END SUBROUTINE

    SUBROUTINE SPLINE_MAP_INIT(map, cap)
        CLASS(SplineMap_t), INTENT(OUT) :: map
        INTEGER,            INTENT(IN)  :: cap

        IF(map%capacity.NE.0) RETURN

        map%capacity = cap
        map%length = 0
        ALLOCATE(map%pairs(cap))
    END SUBROUTINE

    RECURSIVE SUBROUTINE CLEAR_LL(parent)
        TYPE(SplineMapPair_t), POINTER, INTENT(INOUT) :: parent

        IF(ASSOCIATED(parent%next)) THEN
            CALL CLEAR_LL(parent%next)
        ENDIF
        DEALLOCATE(parent)
    END SUBROUTINE

    SUBROUTINE SPLINE_MAP_FREE(map)
        CLASS(SplineMap_t), INTENT(INOUT) :: map
        INTEGER                           :: i

        IF(map%capacity.NE.0) THEN

            DO i = 1, map%capacity
                IF(ASSOCIATED(map%pairs(i)%next)) THEN
                    CALL CLEAR_LL(map%pairs(i)%next)
                ENDIF
            END DO

            DEALLOCATE(map%pairs)
        ENDIF
    END SUBROUTINE

    SUBROUTINE SPLINE_MAP_FIND(map, key, output, error)
        USE iso_fortran_env
        CLASS(SplineMap_t), INTENT(IN), TARGET  :: map
        CHARACTER(*)      , INTENT(IN)          :: key
        TYPE(SplineData_t), INTENT(OUT)         :: output
        LOGICAL, INTENT(OUT)                    :: error
        TYPE(SplineMapPair_t), POINTER          :: pair
        INTEGER                                 :: hash
        INTEGER                                 :: index

        CALL MURMURHASH2_32(key, LEN_TRIM(key), hash)

        index = MODULO(hash, map%capacity)
        pair  => map%pairs(index)

        IF(.NOT.pair%valid) THEN
            error = .TRUE.
            RETURN
        ENDIF

        DO
            IF(TRIM(pair%key).EQ.TRIM(key)) THEN
                error  = .FALSE.
                output = pair%value
                RETURN
            ENDIF

            IF(.NOT.ASSOCIATED(pair%next)) THEN
                error = .TRUE.
                RETURN
            ENDIF
            pair = pair%next
        END DO
    END SUBROUTINE

    SUBROUTINE SPLINE_MAP_INSERT(map, key, input)
        CLASS(SplineMap_t), INTENT(INOUT), TARGET :: map
        CHARACTER(*)      , INTENT(IN)            :: key
        TYPE(SplineData_t), INTENT(IN)            :: input
        TYPE(SplineMapPair_t), POINTER            :: pair
        INTEGER                                   :: hash
        INTEGER                                   :: index

        CALL MURMURHASH2_32(key, LEN_TRIM(key), hash)

        index = MODULO(hash, map%capacity)
        pair  => map%pairs(index)

        ! Insert new (without collision)
        IF(.NOT.pair%valid) THEN
            map%pairs(index) = SplineMapPair_t(key, input, null(), .TRUE.)
            RETURN
        ENDIF
        
        DO
            ! Update
            IF(TRIM(pair%key).EQ.TRIM(key)) THEN
                pair%value = input
                RETURN
            ENDIF

            ! Insert new (with collision)
            IF(.NOT.ASSOCIATED(pair%next)) THEN
                ALLOCATE(pair%next)
                pair%next = SplineMapPair_t(key, input, null(), .TRUE.)
                RETURN
            ENDIF

            pair = pair%next
        END DO
    END SUBROUTINE

END MODULE MOD_INTERPOLATE
