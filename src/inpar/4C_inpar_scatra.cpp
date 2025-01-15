// This file is part of 4C multiphysics licensed under the
// GNU Lesser General Public License v3.0 or later.
//
// See the LICENSE.md file in the top-level for license information.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "4C_inpar_scatra.hpp"

#include "4C_fem_condition_definition.hpp"
#include "4C_inpar_bio.hpp"
#include "4C_inpar_fluid.hpp"
#include "4C_inpar_s2i.hpp"
#include "4C_io_linecomponent.hpp"
#include "4C_linalg_equilibrate.hpp"
#include "4C_linalg_sparseoperator.hpp"
#include "4C_utils_parameter_list.hpp"

FOUR_C_NAMESPACE_OPEN

void Inpar::ScaTra::set_valid_parameters(Teuchos::ParameterList& list)
{
  using namespace Input;
  using Teuchos::setStringToIntegralParameter;
  using Teuchos::tuple;

  Teuchos::ParameterList& scatradyn = list.sublist(
      "SCALAR TRANSPORT DYNAMIC", false, "control parameters for scalar transport problems\n");

  setStringToIntegralParameter<Inpar::ScaTra::SolverType>("SOLVERTYPE", "linear_full",
      "type of scalar transport solver",
      tuple<std::string>("linear_full", "linear_incremental", "nonlinear",
          "nonlinear_multiscale_macrotomicro", "nonlinear_multiscale_macrotomicro_aitken",
          "nonlinear_multiscale_macrotomicro_aitken_dofsplit", "nonlinear_multiscale_microtomacro"),
      tuple<Inpar::ScaTra::SolverType>(solvertype_linear_full, solvertype_linear_incremental,
          solvertype_nonlinear, solvertype_nonlinear_multiscale_macrotomicro,
          solvertype_nonlinear_multiscale_macrotomicro_aitken,
          solvertype_nonlinear_multiscale_macrotomicro_aitken_dofsplit,
          solvertype_nonlinear_multiscale_microtomacro),
      &scatradyn);

  setStringToIntegralParameter<Inpar::ScaTra::TimeIntegrationScheme>("TIMEINTEGR", "One_Step_Theta",
      "Time Integration Scheme",
      tuple<std::string>("Stationary", "One_Step_Theta", "BDF2", "Gen_Alpha"),
      tuple<Inpar::ScaTra::TimeIntegrationScheme>(
          timeint_stationary, timeint_one_step_theta, timeint_bdf2, timeint_gen_alpha),
      &scatradyn);

  Core::Utils::double_parameter("MAXTIME", 1000.0, "Total simulation time", &scatradyn);
  Core::Utils::int_parameter("NUMSTEP", 20, "Total number of time steps", &scatradyn);
  Core::Utils::double_parameter("TIMESTEP", 0.1, "Time increment dt", &scatradyn);
  Core::Utils::double_parameter("THETA", 0.5, "One-step-theta time integration factor", &scatradyn);
  Core::Utils::double_parameter(
      "ALPHA_M", 0.5, "Generalized-alpha time integration factor", &scatradyn);
  Core::Utils::double_parameter(
      "ALPHA_F", 0.5, "Generalized-alpha time integration factor", &scatradyn);
  Core::Utils::double_parameter(
      "GAMMA", 0.5, "Generalized-alpha time integration factor", &scatradyn);
  Core::Utils::int_parameter("RESULTSEVERY", 1, "Increment for writing solution", &scatradyn);
  Core::Utils::int_parameter("RESTARTEVERY", 1, "Increment for writing restart", &scatradyn);
  Core::Utils::int_parameter("MATID", -1, "Material ID for automatic mesh generation", &scatradyn);

  setStringToIntegralParameter<Inpar::ScaTra::VelocityField>("VELOCITYFIELD", "zero",
      "type of velocity field used for scalar transport problems",
      tuple<std::string>("zero", "function", "Navier_Stokes"),
      tuple<Inpar::ScaTra::VelocityField>(velocity_zero, velocity_function, velocity_Navier_Stokes),
      &scatradyn);

  Core::Utils::int_parameter(
      "VELFUNCNO", -1, "function number for scalar transport velocity field", &scatradyn);

  {
    // a standard Teuchos::tuple can have at maximum 10 entries! We have to circumvent this here.
    Teuchos::Tuple<std::string, 13> name;
    Teuchos::Tuple<Inpar::ScaTra::InitialField, 13> label;
    name[0] = "zero_field";
    label[0] = initfield_zero_field;
    name[1] = "field_by_function";
    label[1] = initfield_field_by_function;
    name[2] = "field_by_condition";
    label[2] = initfield_field_by_condition;
    name[3] = "disturbed_field_by_function";
    label[3] = initfield_disturbed_field_by_function;
    name[4] = "1D_DISCONTPV";
    label[4] = initfield_discontprogvar_1D;
    name[5] = "FLAME_VORTEX_INTERACTION";
    label[5] = initfield_flame_vortex_interaction;
    name[6] = "RAYTAYMIXFRAC";
    label[6] = initfield_raytaymixfrac;
    name[7] = "L_shaped_domain";
    label[7] = initfield_Lshapeddomain;
    name[8] = "facing_flame_fronts";
    label[8] = initfield_facing_flame_fronts;
    name[9] = "oracles_flame";
    label[9] = initfield_oracles_flame;
    name[10] = "high_forced_hit";
    label[10] = initialfield_forced_hit_high_Sc;
    name[11] = "low_forced_hit";
    label[11] = initialfield_forced_hit_low_Sc;
    name[12] = "algebraic_field_dependence";
    label[12] = initialfield_algebraic_field_dependence;

    setStringToIntegralParameter<Inpar::ScaTra::InitialField>("INITIALFIELD", "zero_field",
        "Initial Field for scalar transport problem", name, label, &scatradyn);
  }

  Core::Utils::int_parameter(
      "INITFUNCNO", -1, "function number for scalar transport initial field", &scatradyn);

  Core::Utils::bool_parameter("SPHERICALCOORDS", "No", "use of spherical coordinates", &scatradyn);

  setStringToIntegralParameter<Inpar::ScaTra::CalcError>("CALCERROR", "No",
      "compute error compared to analytical solution",
      tuple<std::string>("No", "Kwok_Wu", "ConcentricCylinders", "Electroneutrality",
          "error_by_function", "error_by_condition", "SphereDiffusion", "AnalyticSeries"),
      tuple<Inpar::ScaTra::CalcError>(calcerror_no, calcerror_Kwok_Wu, calcerror_cylinder,
          calcerror_electroneutrality, calcerror_byfunction, calcerror_bycondition,
          calcerror_spherediffusion, calcerror_AnalyticSeries),
      &scatradyn);

  Core::Utils::int_parameter(
      "CALCERRORNO", -1, "function number for scalar transport error computation", &scatradyn);

  setStringToIntegralParameter<Inpar::ScaTra::FluxType>("CALCFLUX_DOMAIN", "No",
      "output of diffusive/total flux vectors inside domain",
      tuple<std::string>("No", "total", "diffusive"),
      tuple<Inpar::ScaTra::FluxType>(flux_none, flux_total, flux_diffusive), &scatradyn);

  Core::Utils::bool_parameter("CALCFLUX_DOMAIN_LUMPED", "Yes",
      "perform approximate domain flux calculation involving matrix lumping", &scatradyn);

  setStringToIntegralParameter<Inpar::ScaTra::FluxType>("CALCFLUX_BOUNDARY", "No",
      "output of convective/diffusive/total flux vectors on boundary",
      tuple<std::string>("No", "total", "diffusive", "convective"),
      tuple<Inpar::ScaTra::FluxType>(flux_none, flux_total, flux_diffusive, flux_convective),
      &scatradyn);

  Core::Utils::bool_parameter("CALCFLUX_BOUNDARY_LUMPED", "Yes",
      "perform approximate boundary flux calculation involving matrix lumping", &scatradyn);

  setNumericStringParameter("WRITEFLUX_IDS", "-1",
      "Write diffusive/total flux vector fields for these scalar fields only (starting with 1)",
      &scatradyn);

  setStringToIntegralParameter<Inpar::ScaTra::OutputScalarType>("OUTPUTSCALARS", "none",
      "Output of total and mean values for transported scalars",
      tuple<std::string>("none", "entire_domain", "by_condition", "entire_domain_and_by_condition"),
      tuple<Inpar::ScaTra::OutputScalarType>(outputscalars_none, outputscalars_entiredomain,
          outputscalars_condition, outputscalars_entiredomain_condition),
      &scatradyn);
  Core::Utils::bool_parameter(
      "OUTPUTSCALARSMEANGRAD", "No", "Output of mean gradient of scalars", &scatradyn);
  Core::Utils::bool_parameter(
      "OUTINTEGRREAC", "No", "Output of integral reaction values", &scatradyn);
  Core::Utils::bool_parameter(
      "OUTPUT_GMSH", "No", "Do you want to write Gmsh postprocessing files?", &scatradyn);

  Core::Utils::bool_parameter("MATLAB_STATE_OUTPUT", "No",
      "Do you want to write the state solution to Matlab file?", &scatradyn);

  setStringToIntegralParameter<Inpar::ScaTra::ConvForm>("CONVFORM", "convective",
      "form of convective term", tuple<std::string>("convective", "conservative"),
      tuple<Inpar::ScaTra::ConvForm>(convform_convective, convform_conservative), &scatradyn);

  Core::Utils::bool_parameter(
      "NEUMANNINFLOW", "no", "Flag to (de)activate potential Neumann inflow term(s)", &scatradyn);

  Core::Utils::bool_parameter("CONV_HEAT_TRANS", "no",
      "Flag to (de)activate potential convective heat transfer boundary conditions", &scatradyn);

  Core::Utils::bool_parameter(
      "SKIPINITDER", "no", "Flag to skip computation of initial time derivative", &scatradyn);

  setStringToIntegralParameter<Inpar::ScaTra::FSSUGRDIFF>("FSSUGRDIFF", "No",
      "fine-scale subgrid diffusivity",
      tuple<std::string>("No", "artificial", "Smagorinsky_all", "Smagorinsky_small"),
      tuple<Inpar::ScaTra::FSSUGRDIFF>(fssugrdiff_no, fssugrdiff_artificial,
          fssugrdiff_smagorinsky_all, fssugrdiff_smagorinsky_small),
      &scatradyn);

  // flag for output of performance statistics associated with nonlinear solver into *.csv file
  Core::Utils::bool_parameter("ELECTROMAGNETICDIFFUSION", "No",
      "flag to activate electromagnetic diffusion problems", &scatradyn);

  // Current density source function for EMD problems
  Core::Utils::int_parameter("EMDSOURCE", -1, "Current density source", &scatradyn);

  setStringToIntegralParameter<Inpar::FLUID::MeshTying>("MESHTYING", "no",
      "Flag to (de)activate mesh tying algorithm",
      tuple<std::string>("no", "Condensed_Smat", "Condensed_Bmat", "Condensed_Bmat_merged"),
      tuple<Inpar::FLUID::MeshTying>(Inpar::FLUID::no_meshtying, Inpar::FLUID::condensed_smat,
          Inpar::FLUID::condensed_bmat, Inpar::FLUID::condensed_bmat_merged),
      &scatradyn);

  // Type of coupling strategy between the two fields
  setStringToIntegralParameter<Inpar::ScaTra::FieldCoupling>("FIELDCOUPLING", "matching",
      "Type of coupling strategy between fields", tuple<std::string>("matching", "volmortar"),
      tuple<Inpar::ScaTra::FieldCoupling>(coupling_match, coupling_volmortar), &scatradyn);

  // linear solver id used for scalar transport/elch problems
  Core::Utils::int_parameter(
      "LINEAR_SOLVER", -1, "number of linear solver used for scalar transport/elch...", &scatradyn);
  // linear solver id used for l2 projection problems (e.g. gradient projections)
  Core::Utils::int_parameter("L2_PROJ_LINEAR_SOLVER", -1,
      "number of linear solver used for l2-projection sub-problems", &scatradyn);

  // flag for equilibration of global system of equations
  setStringToIntegralParameter<Core::LinAlg::EquilibrationMethod>("EQUILIBRATION", "none",
      "flag for equilibration of global system of equations",
      tuple<std::string>("none", "rows_full", "rows_maindiag", "columns_full", "columns_maindiag",
          "rowsandcolumns_full", "rowsandcolumns_maindiag"),
      tuple<Core::LinAlg::EquilibrationMethod>(Core::LinAlg::EquilibrationMethod::none,
          Core::LinAlg::EquilibrationMethod::rows_full,
          Core::LinAlg::EquilibrationMethod::rows_maindiag,
          Core::LinAlg::EquilibrationMethod::columns_full,
          Core::LinAlg::EquilibrationMethod::columns_maindiag,
          Core::LinAlg::EquilibrationMethod::rowsandcolumns_full,
          Core::LinAlg::EquilibrationMethod::rowsandcolumns_maindiag),
      &scatradyn);

  // type of global system matrix in global system of equations
  setStringToIntegralParameter<Core::LinAlg::MatrixType>("MATRIXTYPE", "sparse",
      "type of global system matrix in global system of equations",
      tuple<std::string>("sparse", "block_condition", "block_condition_dof"),
      tuple<Core::LinAlg::MatrixType>(Core::LinAlg::MatrixType::sparse,
          Core::LinAlg::MatrixType::block_condition, Core::LinAlg::MatrixType::block_condition_dof),
      &scatradyn);

  // flag for natural convection effects
  Core::Utils::bool_parameter(
      "NATURAL_CONVECTION", "No", "Include natural convection effects", &scatradyn);

  // parameters for finite difference check
  setStringToIntegralParameter<Inpar::ScaTra::FdCheck>("FDCHECK", "none",
      "flag for finite difference check: none, local, or global",
      tuple<std::string>("none",
          "global",           // perform finite difference check on time integrator level
          "global_extended",  // perform finite difference check on time integrator level for
                              // extended system matrix (e.g., involving Lagrange multipliers or
                              // interface layer thicknesses)
          "local"             // perform finite difference check on element level
          ),
      tuple<Inpar::ScaTra::FdCheck>(
          fdcheck_none, fdcheck_global, fdcheck_global_extended, fdcheck_local),
      &scatradyn);
  Core::Utils::double_parameter("FDCHECKEPS", 1.e-6,
      "dof perturbation magnitude for finite difference check (1.e-6 seems to work very well, "
      "whereas smaller values don't)",
      &scatradyn);
  Core::Utils::double_parameter(
      "FDCHECKTOL", 1.e-6, "relative tolerance for finite difference check", &scatradyn);

  // parameter for optional computation of domain and boundary integrals, i.e., of surface areas and
  // volumes associated with specified nodesets
  setStringToIntegralParameter<Inpar::ScaTra::ComputeIntegrals>("COMPUTEINTEGRALS", "none",
      "flag for optional computation of domain integrals",
      tuple<std::string>("none", "initial", "repeated"),
      tuple<Inpar::ScaTra::ComputeIntegrals>(
          computeintegrals_none, computeintegrals_initial, computeintegrals_repeated),
      &scatradyn);

  // parameter for using p-adpativity and semi-implicit evaluation of the reaction term (at the
  // moment only used for HDG and cardiac monodomain problems)
  Core::Utils::bool_parameter("PADAPTIVITY", "no", "Flag to (de)activate p-adativity", &scatradyn);
  Core::Utils::double_parameter("PADAPTERRORTOL", 1e-6,
      "The error tolerance to calculate the variation of the elemental degree", &scatradyn);
  Core::Utils::double_parameter("PADAPTERRORBASE", 1.66,
      "The error tolerance base to calculate the variation of the elemental degree", &scatradyn);
  Core::Utils::int_parameter(
      "PADAPTDEGREEMAX", 4, "The max. degree of the shape functions", &scatradyn);
  Core::Utils::bool_parameter("SEMIIMPLICIT", "no",
      "Flag to (de)activate semi-implicit calculation of the reaction term", &scatradyn);

  // flag for output of performance statistics associated with linear solver into *.csv file
  Core::Utils::bool_parameter("OUTPUTLINSOLVERSTATS", "No",
      "flag for output of performance statistics associated with linear solver into csv file",
      &scatradyn);

  // flag for output of performance statistics associated with nonlinear solver into *.csv file
  Core::Utils::bool_parameter("OUTPUTNONLINSOLVERSTATS", "No",
      "flag for output of performance statistics associated with nonlinear solver into csv file",
      &scatradyn);

  // flag for point-based null space calculation
  Core::Utils::bool_parameter(
      "NULLSPACE_POINTBASED", "No", "flag for point-based null space calculation", &scatradyn);

  // flag for adaptive time stepping
  Core::Utils::bool_parameter(
      "ADAPTIVE_TIMESTEPPING", "No", "flag for adaptive time stepping", &scatradyn);

  /*----------------------------------------------------------------------*/
  Teuchos::ParameterList& scatra_nonlin = scatradyn.sublist(
      "NONLINEAR", false, "control parameters for solving nonlinear SCATRA problems\n");

  Core::Utils::int_parameter("ITEMAX", 10, "max. number of nonlin. iterations", &scatra_nonlin);
  Core::Utils::double_parameter("CONVTOL", 1e-6, "Tolerance for convergence check", &scatra_nonlin);
  Core::Utils::int_parameter("ITEMAX_OUTER", 10,
      "Maximum number of outer iterations in partitioned coupling schemes (natural convection, "
      "multi-scale simulations etc.)",
      &scatra_nonlin);
  Core::Utils::double_parameter("CONVTOL_OUTER", 1e-6,
      "Convergence check tolerance for outer loop in partitioned coupling schemes (natural "
      "convection, multi-scale simulations etc.)",
      &scatra_nonlin);
  Core::Utils::bool_parameter("EXPLPREDICT", "no",
      "do an explicit predictor step before starting nonlinear iteration", &scatra_nonlin);
  Core::Utils::double_parameter("ABSTOLRES", 1e-14,
      "Absolute tolerance for deciding if residual of nonlinear problem is already zero",
      &scatra_nonlin);

  // convergence criteria adaptivity
  Core::Utils::bool_parameter("ADAPTCONV", "No",
      "Switch on adaptive control of linear solver tolerance for nonlinear solution",
      &scatra_nonlin);
  Core::Utils::double_parameter("ADAPTCONV_BETTER", 0.1,
      "The linear solver shall be this much better than the current nonlinear residual in the "
      "nonlinear convergence limit",
      &scatra_nonlin);

  /*----------------------------------------------------------------------*/
  Teuchos::ParameterList& scatradyn_stab = scatradyn.sublist("STABILIZATION", false,
      "control parameters for the stabilization of scalar transport problems");

  // this parameter governs type of stabilization
  setStringToIntegralParameter<Inpar::ScaTra::StabType>("STABTYPE", "SUPG",
      "type of stabilization (if any)",
      tuple<std::string>("no_stabilization", "SUPG", "GLS", "USFEM", "centered", "upwind"),
      tuple<std::string>(
          "Do not use any stabilization -> only reasonable for low-Peclet-number flows", "Use SUPG",
          "Use GLS", "Use USFEM", "Use centered scheme", "Use upwinded scheme"),
      tuple<Inpar::ScaTra::StabType>(stabtype_no_stabilization, stabtype_SUPG, stabtype_GLS,
          stabtype_USFEM, stabtype_hdg_centered, stabtype_hdg_upwind),
      &scatradyn_stab);

  // this parameter governs whether subgrid-scale velocity is included
  Core::Utils::bool_parameter(
      "SUGRVEL", "no", "potential incorporation of subgrid-scale velocity", &scatradyn_stab);

  // this parameter governs whether all-scale subgrid diffusivity is included
  Core::Utils::bool_parameter("ASSUGRDIFF", "no",
      "potential incorporation of all-scale subgrid diffusivity (a.k.a. discontinuity-capturing) "
      "term",
      &scatradyn_stab);

  // this parameter selects the tau definition applied
  setStringToIntegralParameter<Inpar::ScaTra::TauType>("DEFINITION_TAU", "Franca_Valentin",
      "Definition of tau",
      tuple<std::string>("Taylor_Hughes_Zarins", "Taylor_Hughes_Zarins_wo_dt", "Franca_Valentin",
          "Franca_Valentin_wo_dt", "Shakib_Hughes_Codina", "Shakib_Hughes_Codina_wo_dt", "Codina",
          "Codina_wo_dt", "Franca_Madureira_Valentin", "Franca_Madureira_Valentin_wo_dt",
          "Exact_1D", "Zero", "Numerical_Value"),
      tuple<Inpar::ScaTra::TauType>(tau_taylor_hughes_zarins, tau_taylor_hughes_zarins_wo_dt,
          tau_franca_valentin, tau_franca_valentin_wo_dt, tau_shakib_hughes_codina,
          tau_shakib_hughes_codina_wo_dt, tau_codina, tau_codina_wo_dt,
          tau_franca_madureira_valentin, tau_franca_madureira_valentin_wo_dt, tau_exact_1d,
          tau_zero, tau_numerical_value),
      &scatradyn_stab);

  // this parameter selects the characteristic element length for tau for all
  // stabilization parameter definitions requiring such a length
  setStringToIntegralParameter<Inpar::ScaTra::CharEleLength>("CHARELELENGTH", "streamlength",
      "Characteristic element length for tau",
      tuple<std::string>("streamlength", "volume_equivalent_diameter", "root_of_volume"),
      tuple<Inpar::ScaTra::CharEleLength>(streamlength, volume_equivalent_diameter, root_of_volume),
      &scatradyn_stab);

  // this parameter selects the all-scale subgrid-diffusivity definition applied
  setStringToIntegralParameter<Inpar::ScaTra::AssgdType>("DEFINITION_ASSGD", "artificial_linear",
      "Definition of (all-scale) subgrid diffusivity",
      tuple<std::string>("artificial_linear", "artificial_linear_reinit",
          "Hughes_etal_86_nonlinear", "Tezduyar_Park_86_nonlinear",
          "Tezduyar_Park_86_nonlinear_wo_phizero", "doCarmo_Galeao_91_nonlinear",
          "Almeida_Silva_97_nonlinear", "YZbeta_nonlinear", "Codina_nonlinear"),
      tuple<std::string>("classical linear artificial subgrid-diffusivity",
          "simple linear artificial subgrid-diffusivity const*h",
          "nonlinear isotropic according to Hughes et al. (1986)",
          "nonlinear isotropic according to Tezduyar and Park (1986)",
          "nonlinear isotropic according to Tezduyar and Park (1986) without user parameter "
          "phi_zero",
          "nonlinear isotropic according to doCarmo and Galeao (1991)",
          "nonlinear isotropic according to Almeida and Silva (1997)", "nonlinear YZ beta model",
          "nonlinear isotropic according to Codina"),
      tuple<Inpar::ScaTra::AssgdType>(assgd_artificial, assgd_lin_reinit, assgd_hughes,
          assgd_tezduyar, assgd_tezduyar_wo_phizero, assgd_docarmo, assgd_almeida, assgd_yzbeta,
          assgd_codina),
      &scatradyn_stab);

  // this parameter selects the location where tau is evaluated
  setStringToIntegralParameter<Inpar::ScaTra::EvalTau>("EVALUATION_TAU", "element_center",
      "Location where tau is evaluated", tuple<std::string>("element_center", "integration_point"),
      tuple<std::string>("evaluate tau at element center", "evaluate tau at integration point"),
      tuple<Inpar::ScaTra::EvalTau>(evaltau_element_center, evaltau_integration_point),
      &scatradyn_stab);

  // this parameter selects the location where the material law is evaluated
  // (does not fit here very well, but parameter transfer is easier)
  setStringToIntegralParameter<Inpar::ScaTra::EvalMat>("EVALUATION_MAT", "element_center",
      "Location where material law is evaluated",
      tuple<std::string>("element_center", "integration_point"),
      tuple<std::string>(
          "evaluate material law at element center", "evaluate material law at integration point"),
      tuple<Inpar::ScaTra::EvalMat>(evalmat_element_center, evalmat_integration_point),
      &scatradyn_stab);

  // this parameter selects methods for improving consistency of stabilization terms
  setStringToIntegralParameter<Inpar::ScaTra::Consistency>("CONSISTENCY", "no",
      "improvement of consistency for stabilization",
      tuple<std::string>("no", "L2_projection_lumped"),
      tuple<std::string>("inconsistent", "L2 projection with lumped mass matrix"),
      tuple<Inpar::ScaTra::Consistency>(consistency_no, consistency_l2_projection_lumped),
      &scatradyn_stab);

  // this parameter defines the numerical value, if stabilization with numerical values is used
  Core::Utils::double_parameter(
      "TAU_VALUE", 0.0, "Numerical value for tau for stabilization", &scatradyn_stab);

  // ----------------------------------------------------------------------
  // artery mesh tying
  Teuchos::ParameterList& scatradyn_art =
      scatradyn.sublist("ARTERY COUPLING", false, "Parameters for artery mesh tying");

  setStringToIntegralParameter<Inpar::ArteryNetwork::ArteryPoroMultiphaseScatraCouplingMethod>(
      "ARTERY_COUPLING_METHOD", "None", "Coupling method for artery coupling.",
      tuple<std::string>("None", "Nodal", "GPTS", "MP", "NTP"),
      tuple<std::string>("none", "Nodal Coupling", "Gauss-Point-To-Segment Approach",
          "Mortar Penalty Approach", "1D node-to-point in 2D/3D Approach"),
      tuple<Inpar::ArteryNetwork::ArteryPoroMultiphaseScatraCouplingMethod>(
          Inpar::ArteryNetwork::ArteryPoroMultiphaseScatraCouplingMethod::none,   // none
          Inpar::ArteryNetwork::ArteryPoroMultiphaseScatraCouplingMethod::nodal,  // Nodal Coupling
          Inpar::ArteryNetwork::ArteryPoroMultiphaseScatraCouplingMethod::
              gpts,  // Gauss-Point-To-Segment
          Inpar::ArteryNetwork::ArteryPoroMultiphaseScatraCouplingMethod::mp,  // Mortar Penalty
          Inpar::ArteryNetwork::ArteryPoroMultiphaseScatraCouplingMethod::ntp  // 1D node-to-point
                                                                               // in 2D/3D
          ),
      &scatradyn_art);

  // penalty parameter
  Core::Utils::double_parameter(
      "PENALTY", 1000.0, "Penalty parameter for line-based coupling", &scatradyn_art);

  // coupled artery dofs for mesh tying
  setNumericStringParameter(
      "COUPLEDDOFS_ARTSCATRA", "-1.0", "coupled artery dofs for mesh tying", &scatradyn_art);

  // coupled porofluid dofs for mesh tying
  setNumericStringParameter(
      "COUPLEDDOFS_SCATRA", "-1.0", "coupled porofluid dofs for mesh tying", &scatradyn_art);

  // functions for coupling (arteryscatra part)
  setNumericStringParameter(
      "REACFUNCT_ART", "-1", "functions for coupling (arteryscatra part)", &scatradyn_art);

  // scale for coupling (arteryscatra part)
  setNumericStringParameter(
      "SCALEREAC_ART", "0", "scale for coupling (arteryscatra part)", &scatradyn_art);

  // functions for coupling (scatra part)
  setNumericStringParameter(
      "REACFUNCT_CONT", "-1", "functions for coupling (scatra part)", &scatradyn_art);

  // scale for coupling (scatra part)
  setNumericStringParameter(
      "SCALEREAC_CONT", "0", "scale for coupling (scatra part)", &scatradyn_art);

  // ----------------------------------------------------------------------
  Teuchos::ParameterList& scatradyn_external_force =
      scatradyn.sublist("EXTERNAL FORCE", false, "Parameters for magnetics");

  // Flag for external force
  Core::Utils::bool_parameter(
      "EXTERNAL_FORCE", "No", "Flag to activate external force", &scatradyn_external_force);

  // Function ID for external force
  Core::Utils::int_parameter(
      "FORCE_FUNCTION_ID", -1, "Function ID for external force", &scatradyn_external_force);

  // Function ID for mobility of the scalar
  Core::Utils::int_parameter("INTRINSIC_MOBILITY_FUNCTION_ID", -1,
      "Function ID for intrinsic mobility", &scatradyn_external_force);
}



void Inpar::ScaTra::set_valid_conditions(
    std::vector<std::shared_ptr<Core::Conditions::ConditionDefinition>>& condlist)
{
  using namespace Input;

  /*--------------------------------------------------------------------*/
  // Boundary flux evaluation condition for scalar transport
  std::shared_ptr<Core::Conditions::ConditionDefinition> linebndryfluxeval =
      std::make_shared<Core::Conditions::ConditionDefinition>("SCATRA FLUX CALC LINE CONDITIONS",
          "ScaTraFluxCalc", "Scalar Transport Boundary Flux Calculation",
          Core::Conditions::ScaTraFluxCalc, true, Core::Conditions::geometry_type_line);
  std::shared_ptr<Core::Conditions::ConditionDefinition> surfbndryfluxeval =
      std::make_shared<Core::Conditions::ConditionDefinition>("SCATRA FLUX CALC SURF CONDITIONS",
          "ScaTraFluxCalc", "Scalar Transport Boundary Flux Calculation",
          Core::Conditions::ScaTraFluxCalc, true, Core::Conditions::geometry_type_surface);
  condlist.emplace_back(linebndryfluxeval);
  condlist.emplace_back(surfbndryfluxeval);

  /*--------------------------------------------------------------------*/
  // conditions for calculation of total and mean values of transported scalars
  std::shared_ptr<Core::Conditions::ConditionDefinition> totalandmeanscalarline =
      std::make_shared<Core::Conditions::ConditionDefinition>(
          "DESIGN TOTAL AND MEAN SCALAR LINE CONDITIONS", "TotalAndMeanScalar",
          "calculation of total and mean values of transported scalars",
          Core::Conditions::TotalAndMeanScalar, true, Core::Conditions::geometry_type_line);
  std::shared_ptr<Core::Conditions::ConditionDefinition> totalandmeanscalarsurf =
      std::make_shared<Core::Conditions::ConditionDefinition>(
          "DESIGN TOTAL AND MEAN SCALAR SURF CONDITIONS", "TotalAndMeanScalar",
          "calculation of total and mean values of transported scalars",
          Core::Conditions::TotalAndMeanScalar, true, Core::Conditions::geometry_type_surface);
  std::shared_ptr<Core::Conditions::ConditionDefinition> totalandmeanscalarvol =
      std::make_shared<Core::Conditions::ConditionDefinition>(
          "DESIGN TOTAL AND MEAN SCALAR VOL CONDITIONS", "TotalAndMeanScalar",
          "calculation of total and mean values of transported scalars",
          Core::Conditions::TotalAndMeanScalar, true, Core::Conditions::geometry_type_volume);

  for (const auto& cond : {totalandmeanscalarline, totalandmeanscalarsurf, totalandmeanscalarvol})
  {
    // insert input file line components into condition definitions
    cond->add_component(std::make_shared<Input::SeparatorComponent>("ID"));
    cond->add_component(std::make_shared<Input::IntComponent>("ConditionID"));

    // insert condition definitions into global list of valid condition definitions
    condlist.emplace_back(cond);
  }

  /*--------------------------------------------------------------------*/
  // conditions for calculation of relative error with reference to analytical solution
  std::shared_ptr<Core::Conditions::ConditionDefinition> relerrorline =
      std::make_shared<Core::Conditions::ConditionDefinition>(
          "DESIGN SCATRA RELATIVE ERROR LINE CONDITIONS", "ScatraRelError",
          "calculation of relative error with reference to analytical solution",
          Core::Conditions::ScatraRelError, true, Core::Conditions::geometry_type_line);
  std::shared_ptr<Core::Conditions::ConditionDefinition> relerrorsurf =
      std::make_shared<Core::Conditions::ConditionDefinition>(
          "DESIGN SCATRA RELATIVE ERROR SURF CONDITIONS", "ScatraRelError",
          "calculation of relative error with reference to analytical solution",
          Core::Conditions::ScatraRelError, true, Core::Conditions::geometry_type_surface);
  std::shared_ptr<Core::Conditions::ConditionDefinition> relerrorvol =
      std::make_shared<Core::Conditions::ConditionDefinition>(
          "DESIGN SCATRA RELATIVE ERROR VOL CONDITIONS", "ScatraRelError",
          "calculation of relative error with reference to analytical solution",
          Core::Conditions::ScatraRelError, true, Core::Conditions::geometry_type_volume);

  for (const auto& cond : {relerrorline, relerrorsurf, relerrorvol})
  {
    // insert input file line components into condition definitions
    cond->add_component(std::make_shared<Input::SeparatorComponent>("ID"));
    cond->add_component(std::make_shared<Input::IntComponent>("ConditionID"));
    add_named_int(cond, "Function");

    // insert condition definitions into global list of valid condition definitions
    condlist.emplace_back(cond);
  }

  /*--------------------------------------------------------------------*/
  // Coupling of different scalar transport fields

  std::shared_ptr<Core::Conditions::ConditionDefinition> surfscatracoup =
      std::make_shared<Core::Conditions::ConditionDefinition>(
          "DESIGN SCATRA COUPLING SURF CONDITIONS", "ScaTraCoupling", "ScaTra Coupling",
          Core::Conditions::ScaTraCoupling, true, Core::Conditions::geometry_type_surface);

  add_named_int(surfscatracoup, "NUMSCAL");
  add_named_int_vector(surfscatracoup, "ONOFF", "", "NUMSCAL");
  add_named_int(surfscatracoup, "COUPID");
  add_named_real(surfscatracoup, "PERMCOEF");
  add_named_real(surfscatracoup, "CONDUCT");
  add_named_real(surfscatracoup, "FILTR");
  add_named_bool(surfscatracoup, "WSSON", "flag if wall shear stress coupling is on");
  add_named_real_vector(surfscatracoup, "WSSCOEFFS", "", 2);

  condlist.emplace_back(surfscatracoup);

  /*--------------------------------------------------------------------*/
  // Robin boundary condition for scalar transport problems
  // line
  std::shared_ptr<Core::Conditions::ConditionDefinition> scatrarobinline =
      std::make_shared<Core::Conditions::ConditionDefinition>(
          "DESIGN TRANSPORT ROBIN LINE CONDITIONS", "TransportRobin",
          "Scalar Transport Robin Boundary Condition", Core::Conditions::TransportRobin, true,
          Core::Conditions::geometry_type_line);
  // surface
  std::shared_ptr<Core::Conditions::ConditionDefinition> scatrarobinsurf =
      std::make_shared<Core::Conditions::ConditionDefinition>(
          "DESIGN TRANSPORT ROBIN SURF CONDITIONS", "TransportRobin",
          "Scalar Transport Robin Boundary Condition", Core::Conditions::TransportRobin, true,
          Core::Conditions::geometry_type_surface);

  for (const auto& cond : {scatrarobinline, scatrarobinsurf})
  {
    add_named_int(cond, "NUMSCAL");
    add_named_int_vector(cond, "ONOFF", "", "NUMSCAL");
    add_named_real(cond, "PREFACTOR");
    add_named_real(cond, "REFVALUE");

    condlist.emplace_back(cond);
  }

  /*--------------------------------------------------------------------*/
  // Neumann inflow for SCATRA

  std::shared_ptr<Core::Conditions::ConditionDefinition> linetransportneumanninflow =
      std::make_shared<Core::Conditions::ConditionDefinition>(
          "TRANSPORT NEUMANN INFLOW LINE CONDITIONS", "TransportNeumannInflow",
          "Line Transport Neumann Inflow", Core::Conditions::TransportNeumannInflow, true,
          Core::Conditions::geometry_type_line);
  std::shared_ptr<Core::Conditions::ConditionDefinition> surftransportneumanninflow =
      std::make_shared<Core::Conditions::ConditionDefinition>(
          "TRANSPORT NEUMANN INFLOW SURF CONDITIONS", "TransportNeumannInflow",
          "Surface Transport Neumann Inflow", Core::Conditions::TransportNeumannInflow, true,
          Core::Conditions::geometry_type_surface);

  condlist.emplace_back(linetransportneumanninflow);
  condlist.emplace_back(surftransportneumanninflow);

  /*--------------------------------------------------------------------*/
  // Scatra convective heat transfer (Newton's law of heat transfer)
  std::shared_ptr<Core::Conditions::ConditionDefinition> linetransportthermoconvect =
      std::make_shared<Core::Conditions::ConditionDefinition>(
          "TRANSPORT THERMO CONVECTION LINE CONDITIONS", "TransportThermoConvections",
          "Line Transport Thermo Convections", Core::Conditions::TransportThermoConvections, true,
          Core::Conditions::geometry_type_line);
  std::shared_ptr<Core::Conditions::ConditionDefinition> surftransportthermoconvect =
      std::make_shared<Core::Conditions::ConditionDefinition>(
          "TRANSPORT THERMO CONVECTION SURF CONDITIONS", "TransportThermoConvections",
          "Surface Transport Thermo Convections", Core::Conditions::TransportThermoConvections,
          true, Core::Conditions::geometry_type_surface);

  for (const auto& cond : {linetransportthermoconvect, surftransportthermoconvect})
  {
    // decide here if approximation is sufficient
    // --> Tempn (old temperature T_n)
    // or if the exact solution is needed
    // --> Tempnp (current temperature solution T_n+1) with linearisation
    add_named_selection_component(cond, "temperature_state", "temperature state", "Tempnp",
        Teuchos::tuple<std::string>("Tempnp", "Tempn"),
        Teuchos::tuple<std::string>("Tempnp", "Tempn"));
    add_named_real(cond, "coeff", "heat transfer coefficient h");
    add_named_real(cond, "surtemp", "surrounding (fluid) temperature T_oo");
    add_named_int(cond, "surtempfunct",
        "time curve to increase the surrounding (fluid) temperature T_oo in time", 0, false);
    add_named_int(cond, "funct",
        "time curve to increase the complete boundary condition, i.e., the heat flux", 0, false);

    condlist.emplace_back(cond);
  }

  /*--------------------------------------------------------------------*/
  // conditions for calculation of calculation of heterogeneous reactions
  std::shared_ptr<Core::Conditions::ConditionDefinition> scatraheteroreactionmasterline =
      std::make_shared<Core::Conditions::ConditionDefinition>(
          "DESIGN SCATRA HETEROGENEOUS REACTION LINE CONDITIONS / MASTER",
          "ScatraHeteroReactionMaster", "calculation of heterogeneous reactions",
          Core::Conditions::ScatraHeteroReactionCondMaster, true,
          Core::Conditions::geometry_type_line);
  std::shared_ptr<Core::Conditions::ConditionDefinition> scatraheteroreactionmastersurf =
      std::make_shared<Core::Conditions::ConditionDefinition>(
          "DESIGN SCATRA HETEROGENEOUS REACTION SURF CONDITIONS / MASTER",
          "ScatraHeteroReactionMaster", "calculation of heterogeneous reactions",
          Core::Conditions::ScatraHeteroReactionCondMaster, true,
          Core::Conditions::geometry_type_surface);

  // insert condition definitions into global list of valid condition definitions
  condlist.emplace_back(scatraheteroreactionmasterline);
  condlist.emplace_back(scatraheteroreactionmastersurf);

  /*--------------------------------------------------------------------*/
  // conditions for calculation of calculation of heterogeneous reactions
  std::shared_ptr<Core::Conditions::ConditionDefinition> scatraheteroreactionslaveline =
      std::make_shared<Core::Conditions::ConditionDefinition>(
          "DESIGN SCATRA HETEROGENEOUS REACTION LINE CONDITIONS / SLAVE",
          "ScatraHeteroReactionSlave", "calculation of heterogeneous reactions",
          Core::Conditions::ScatraHeteroReactionCondSlave, true,
          Core::Conditions::geometry_type_line);
  std::shared_ptr<Core::Conditions::ConditionDefinition> scatraheteroreactionslavesurf =
      std::make_shared<Core::Conditions::ConditionDefinition>(
          "DESIGN SCATRA HETEROGENEOUS REACTION SURF CONDITIONS / SLAVE",
          "ScatraHeteroReactionSlave", "calculation of heterogeneous reactions",
          Core::Conditions::ScatraHeteroReactionCondSlave, true,
          Core::Conditions::geometry_type_surface);

  // insert condition definitions into global list of valid condition definitions
  condlist.emplace_back(scatraheteroreactionslaveline);
  condlist.emplace_back(scatraheteroreactionslavesurf);


  /*--------------------------------------------------------------------*/
  // scatra domain partitioning for block preconditioning of global system matrix
  // please note: this is currently only used in combination with scatra-scatra interface coupling
  // however the complete scatra matrix is subdivided into blocks which is not related to the
  // interface coupling at all
  {
    // partitioning of 2D domain into 2D subdomains
    auto scatrasurfpartitioning = std::make_shared<Core::Conditions::ConditionDefinition>(
        "DESIGN SCATRA SURF CONDITIONS / PARTITIONING", "ScatraPartitioning",
        "Domain partitioning of scatra field", Core::Conditions::ScatraPartitioning, false,
        Core::Conditions::geometry_type_surface);

    // partitioning of 3D domain into 3D subdomains
    auto scatravolpartitioning = std::make_shared<Core::Conditions::ConditionDefinition>(
        "DESIGN SCATRA VOL CONDITIONS / PARTITIONING", "ScatraPartitioning",
        "Domain partitioning of scatra field", Core::Conditions::ScatraPartitioning, false,
        Core::Conditions::geometry_type_volume);

    // insert condition definitions into global list of valid condition definitions
    condlist.emplace_back(scatrasurfpartitioning);
    condlist.emplace_back(scatravolpartitioning);
  }
}

FOUR_C_NAMESPACE_CLOSE
