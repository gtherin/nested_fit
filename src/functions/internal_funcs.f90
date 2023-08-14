! Brief : This module contains the internal functions that get compiled to aid in the user-defined functions
!         This gets compiled inside the shared library with the user functions
!         There is no need to link against this with the main nested_fit executable
! Author : César Godinho
! Date   : 09/08/2023

! Compute the real value out of the faddeeva function w(z)
FUNCTION WofzRe(zr, zi)
    IMPLICIT NONE
    REAL(8), INTENT(IN) :: zr, zi
    REAL(8) :: wr, wi
    LOGICAL :: flag
    REAL(8) :: WofzRe
    EXTERNAL WOFZ

    CALL WOFZ(zr, zi, wr, wi, flag)
    WofzRe = wr
    RETURN
END FUNCTION

FUNCTION Interpolate(interpolator_file, x, s)
    USE MOD_INTERPOLATE
    IMPLICIT NONE    
    CHARACTER(128), INTENT(IN) :: interpolator_file
    REAL(8),        INTENT(IN) :: x
    REAL(8),        INTENT(IN) :: s
    REAL(8)                    :: Interpolate
    TYPE(SplineData_t)         :: spline_data
    LOGICAL                    :: error
    INTEGER                    :: ierr

    CALL GlobalSplineMap%find(interpolator_file, spline_data, error)

    ! TODO(César) : There must be a more efficient way of doing this that I am not seeing now
    IF(.NOT.error) THEN
        ! Use spline data
        CALL EVALUATE_SPLINE_DATA(spline_data, x, Interpolate)
        RETURN
    ENDIF
    
    ! Interpolate the new spline data
    CALL INTERPOLATE_FROM_FILE(interpolator_file, spline_data, s)
    CALL GlobalSplineMap%insert(interpolator_file, spline_data)

    CALL GlobalSplineMap%find(interpolator_file, spline_data, error)
    IF(error) THEN
        ! Fatal error
        ! TODO(César) : How do we handle this if we are inside and OpenMP context???
    ELSE
        CALL EVALUATE_SPLINE_DATA(spline_data, x, Interpolate)
        RETURN
    ENDIF
END FUNCTION
