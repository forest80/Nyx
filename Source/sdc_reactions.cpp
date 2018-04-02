
#include "Nyx.H"
#include "Nyx_F.H"

using namespace amrex;
using std::string;

void
Nyx::sdc_reactions (MultiFab& S_old, MultiFab& S_new, MultiFab& D_new, 
                    MultiFab& hydro_src, MultiFab& IR,
                    Real delta_time, Real a_old, Real a_new, int sdc_iter)
{
    BL_PROFILE("Nyx::sdc_reactions()");

    const Real* dx = geom.CellSize();

    compute_new_temp();
    
#ifndef FORCING
    {
      const Real z = 1.0/a_old - 1.0;
      fort_interp_to_this_z(&z);
    }
#endif

    int  min_iter = 100000;
    int  max_iter =      0;

    int  min_iter_grid, max_iter_grid;
    
#ifdef _OPENMP
#pragma omp parallel
#endif
    for (MFIter mfi(S_old,true); mfi.isValid(); ++mfi)
    {
        // Note that this "bx" is only the valid region (unlike for Strang)
      const Box& bx = mfi.tilebox();

        min_iter_grid = 100000;
        max_iter_grid =      0;

        integrate_state_with_source
                (bx.loVect(), bx.hiVect(), 
                 BL_TO_FORTRAN(S_old[mfi]),
                 BL_TO_FORTRAN(S_new[mfi]),
                 BL_TO_FORTRAN(D_old[mfi]),
		 BL_TO_FORTRAN(hydro_src[mfi]),
		 BL_TO_FORTRAN(IR[mfi]),
                 &a_old, &delta_time, &min_iter_grid, &max_iter_grid);

        min_iter = std::min(min_iter,min_iter_grid);
        max_iter = std::max(max_iter,max_iter_grid);
    }

    ParallelDescriptor::ReduceIntMax(max_iter);
    ParallelDescriptor::ReduceIntMin(min_iter);

    amrex::Print() << "Min/Max Number of Iterations in SDC: " << min_iter << " " << max_iter << std::endl;
}
