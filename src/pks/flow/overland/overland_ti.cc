/* -*-  mode: c++; c-default-style: "google"; indent-tabs-mode: nil -*- */

/*
A base two-phase, thermal Richard's equation with water vapor.

Authors: Ethan Coon (ATS version) (ecoon@lanl.gov)
*/

#include "Epetra_FECrsMatrix.h"
#include "EpetraExt_RowMatrixOut.h"

#include "overland.hh"

namespace Amanzi {
namespace Flow {

#if 0
}}
#endif

/* ******************************************************************
 * Calculate f(u, du/dt) = d(s u)/dt + A*u - g.
 ****************************************************************** */
void OverlandFlow::fun( double t_old,
                        double t_new,
                        Teuchos::RCP<TreeVector> u_old,
                        Teuchos::RCP<TreeVector> u_new,
                        Teuchos::RCP<TreeVector> g ) {
  S_inter_->set_time(t_old);
  S_next_ ->set_time(t_new);

  Teuchos::RCP<CompositeVector> u = u_new->data();
  std::cout << "OverlandFlow Residual calculation:" << std::endl;
  std::cout << "  p0: " << (*u)("cell",0,0) << " " << (*u)("face",0,3) << std::endl;
  std::cout << "  p1: " << (*u)("cell",0,9) << " " << (*u)("face",0,29) << std::endl;

  // pointer-copy temperature into state and update any auxilary data
  solution_to_state(u_new, S_next_);
  UpdateSecondaryVariables_(S_next_);

  // update boundary conditions
  bc_pressure_->Compute(t_new);
  bc_flux_    ->Compute(t_new);
  UpdateBoundaryConditions_(S_next_, *u);

  // zero out residual
  Teuchos::RCP<CompositeVector> res = g->data();
  res->PutScalar(0.0);

  // diffusion term, treated implicitly
  ApplyDiffusion_(S_next_, res);
  std::cout << "  res0 (after diffusion): " << (*res)("cell",0,0) << " " << (*res)("face",0,3) << std::endl;
  std::cout << "  res1 (after diffusion): " << (*res)("cell",0,99) << " " << (*res)("face",0,29) << std::endl;

  // accumulation term
  AddAccumulation_(res);
  std::cout << "  res0 (after accumulation): " << (*res)("cell",0,0) << " " << (*res)("face",0,3) << std::endl;
  std::cout << "  res1 (after accumulation): " << (*res)("cell",0,99) << " " << (*res)("face",0,29) << std::endl;

  // add rhs load value
  AddLoadValue_(S_next_,res);
};

/* ******************************************************************
* Apply preconditioner to u and return the result in Pu.
****************************************************************** */
void OverlandFlow::precon(Teuchos::RCP<const TreeVector> u, Teuchos::RCP<TreeVector> Pu) {
  std::cout << "Precon application:" << std::endl;
  std::cout << "  p0: " << (*u->data())("cell",0,0) << " " << (*u->data())("face",0,3) << std::endl;
  std::cout << "  p1: " << (*u->data())("cell",0,99) << " " << (*u->data())("face",0,29) << std::endl;
  //preconditioner_->ApplyInverse(*u->data(), Pu->data());
  *Pu = *u ;
  std::cout << "  PC*p0: " << (*Pu->data())("cell",0,0) << " " << (*Pu->data())("face",0,3) << std::endl;
  std::cout << "  PC*p1: " << (*Pu->data())("cell",0,99) << " " << (*Pu->data())("face",0,29) << std::endl;
};


/* ******************************************************************
 * computes a norm on u-du and returns the result
 ****************************************************************** */
double OverlandFlow::enorm(Teuchos::RCP<const TreeVector> u,
                           Teuchos::RCP<const TreeVector> du) {
  double enorm_val = 0.0;
  Teuchos::RCP<const Epetra_MultiVector> pres_vec = u->data()->ViewComponent("cell", false);
  Teuchos::RCP<const Epetra_MultiVector> dpres_vec = du->data()->ViewComponent("cell", false);

  for (int lcv=0; lcv!=pres_vec->MyLength(); ++lcv) {
    double tmp = abs((*(*dpres_vec)(0))[lcv])/(atol_ + rtol_*abs((*(*pres_vec)(0))[lcv]));
    enorm_val = std::max<double>(enorm_val, tmp);
  }

  Teuchos::RCP<const Epetra_MultiVector> fpres_vec = u->data()->ViewComponent("face", false);
  Teuchos::RCP<const Epetra_MultiVector> fdpres_vec = du->data()->ViewComponent("face", false);

  for (int lcv=0; lcv!=fpres_vec->MyLength(); ++lcv) {
    double tmp = abs((*(*fdpres_vec)(0))[lcv])/(atol_ + rtol_*abs((*(*fpres_vec)(0))[lcv]));
    enorm_val = std::max<double>(enorm_val, tmp);
  }

#ifdef HAVE_MPI
  double buf = enorm_val;
  MPI_Allreduce(&buf, &enorm_val, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);
#endif

  return enorm_val;
};

/* ******************************************************************
 * Compute new preconditioner B(p, dT_prec).
 ****************************************************************** */
void OverlandFlow::update_precon(double t, Teuchos::RCP<const TreeVector> up, double h) {
  std::cout << "Precon update at t = " << t << std::endl;
  // update state with the solution up.
  S_next_->set_time(t);
  PK::solution_to_state(up, S_next_);
  UpdateSecondaryVariables_(S_next_);

  // update with accumulation terms
  Teuchos::RCP<const CompositeVector> pres = S_next_->GetFieldData("overland_pressure");

  // update boundary conditions
  bc_pressure_->Compute(S_next_->time());
  bc_flux_    ->Compute(S_next_->time());
  UpdateBoundaryConditions_( S_next_, *pres );

  // update the rel perm according to the scheme of choice
  UpdatePermeabilityData_(S_next_);
  Teuchos::RCP<const CompositeVector> cond =
    S_next_->GetFieldData("upwind_overland_conductivity");

  preconditioner_->CreateMFDstiffnessMatrices(*cond);
  preconditioner_->CreateMFDrhsVectors();

  Teuchos::RCP<const CompositeVector> cell_volume = S_next_->GetFieldData("cell_volume");

  std::vector<double>& Acc_cells = preconditioner_->Acc_cells();
  std::vector<Teuchos::SerialDenseMatrix<int, double> >& Aff_cells = preconditioner_->Aff_cells();
  std::vector<Epetra_SerialDenseVector>& Acf_cells = preconditioner_->Acf_cells();
  std::vector<Epetra_SerialDenseVector>& Afc_cells = preconditioner_->Afc_cells();
  std::vector<double>& Fc_cells = preconditioner_->Fc_cells();
  std::vector<Epetra_SerialDenseVector>& Ff_cells = preconditioner_->Ff_cells();

  preconditioner_->ApplyBoundaryConditions(bc_markers_, bc_values_);
  preconditioner_->AssembleGlobalMatrices();
  preconditioner_->ComputeSchurComplement(bc_markers_, bc_values_);

  // need to update the PC's RHS -- it currently has z+p, but should have just p

  // Code to dump Schur complement to check condition number
  /*
    Teuchos::RCP<Epetra_FECrsMatrix> sc = preconditioner_->Schur();
    std::stringstream filename_s;
    filename_s << "schur_" << S_next_->cycle() << ".txt";
    //a  std::string filename = filename_s.str();
    EpetraExt::RowMatrixToMatlabFile(filename_s.str().c_str(), *sc);
    std::cout << "updated precon " << S_next_->cycle() << std::endl;
  */

  preconditioner_->UpdateMLPreconditioner();
};

}  // namespace Flow
}  // namespace Amanzi