#ifndef __CHASSIS_CAR_3DOF_H__
#define __CHASSIS_CAR_3DOF_H__

#include "src/core/foundation/fastest_lap_exception.h"
#include "chassis.h"
#include <array>
#include <vector>

//!     Car chassis model with two small pitch and roll rotations
//!     ---------------------------------------------------------
//!
//!     The chassis frame is parallel to the road projection of CoM frame.
//! Therefore, the small rotations are not included in the frame motions
//! and will be manually included through updates of the positions of the 
//! axles frames.
//!  * The chassis frame coordinates are (0,0,z-h) and velocity (0,0,dz). Parallel to the road frame
//!  * The axles are located at (x_ax,0,z_ax + mu.x_ax) with velocity (0,0,dmu.x_ax)
//!  @param FrontAxle_t: type of the front axle
//!  @param RearAxle_t: type of the rear axle
//!  @param STATE0: index of the first state variable defined here
//!  @param CONTROL0: index of the first control variable defined here
template<typename Timeseries_t, typename FrontAxle_t, typename RearAxle_t, size_t STATE0, size_t CONTROL0>
class Chassis_car_3dof : public Chassis<Timeseries_t,FrontAxle_t, RearAxle_t, STATE0,CONTROL0>
{
 public:

    //! Type of the chassic of which this class derives
    using base_type             = Chassis<Timeseries_t,FrontAxle_t, RearAxle_t,STATE0,CONTROL0>;

    //! Type of the front axle
    using Front_axle_type       = FrontAxle_t;

    //! Type of the rear axle
    using Rear_axle_type        = RearAxle_t;

    //! Type of the front left tyre
    using Front_left_tire_type  = typename FrontAxle_t::Tire_left_type;

    //! Type of the front right tire
    using Front_right_tire_type = typename FrontAxle_t::Tire_right_type;

    //! Type of the rear left tire
    using Rear_left_tire_type   = typename RearAxle_t::Tire_left_type;

    //! Type of the rear right tire
    using Rear_right_tire_type  = typename RearAxle_t::Tire_right_type;
    
    //! The two axles: FRONT and REAR
    enum Axles { FRONT, REAR }; 

    // State variables: none
    enum State { STATE_END = base_type::STATE_END };

    //! Control variables: throttle/brake
    enum Controls  { ITHROTTLE = base_type::CONTROL_END, IBRAKE_BIAS, CONTROL_END };

    //! Algebraic variables: the four vertical forces
    enum Algebraic { IFZFL, IFZFR, IFZRL, IFZRR, ALGEBRAIC_END };

    constexpr static size_t NALGEBRAIC = ALGEBRAIC_END;

    //! Default constructor
    Chassis_car_3dof();

    //! Constructor from axles and extra parameters
    //! @param[in] front_axle: Front axle
    //! @param[in] rear_axle: Rear axle
    //! @param[in] parameters: map containing mechanical/geometrical parameters by name
    //! @param[in] path: path to find its parameters on the map
    Chassis_car_3dof(const FrontAxle_t& front_axle, 
                const RearAxle_t& rear_axle,
                Xml_document& database,
                const std::string& path=""
               );

    //! Constructor only from parameter map and tire types
    //! @param[in] parameters: map with all mechanical/geometrical parameters by name.
    //!                        It must contain the parameters to define the tires and axles
    //! @param[in] front_left_tire_type: Type of the front left tire
    //! @param[in] front_right_tire_type: Type of the front right tire type
    //! @param[in] rear_left_tire_type: Type of the rear left tire
    //! @param[in] rear_right_tire_type: Type of the rear right tire type
    Chassis_car_3dof(Xml_document& database);

    //! Modifyer to set a parameter from the database
    template<typename T>
    void set_parameter(const std::string& parameter, const T value);

    //! Fill the corresponding nodes of an xml document
    void fill_xml(Xml_document& doc) const;

    //! Update the chassis: update the axles to get forces and compute accelerations
    //! @param[in] x: x-coordinate of the road frame [m]
    //! @param[in] y: y-coordinate of the road frame [m]
    //! @param[in] psi: yaw angle of the road frame [rad]
    void update(Timeseries_t x, Timeseries_t y, Timeseries_t psi);

    //! Set the chassis state variables from direct values
    //      --- Set by the parent class ---
    //! @param[in] u: road frame x-velocity (in road frame) [m/s]
    //! @param[in] v: road frame y-velocity (in road frame) [m/s]
    //! @param[in] omega: road frame yaw speed [rad/s]
    void set_state(Timeseries_t u, Timeseries_t v, Timeseries_t omega); 

    //! Get the CoM position in road frame
    Vector3d<Timeseries_t> get_com_position() const { return _x_com; }

    //! Get the CoM velocity in road frame
    Vector3d<Timeseries_t> get_com_velocity() const { return { 0.0, 0.0, 0.0 }; }

    //! Computes the front axle position in chassis frame. It is constant in the 3DOF model
    Vector3d<Timeseries_t> get_front_axle_position() const { return {_x_front_axle[0], _x_front_axle[1], _x_front_axle[2]}; }

    //! Computes the rear axle position in chassis frame. It is constant in the 3DOF model
    Vector3d<Timeseries_t> get_rear_axle_position() const { return {_x_rear_axle[0], _x_rear_axle[1], _x_rear_axle[2]}; }

    //! Get the chassis CoM absolute acceleration in road frame
    Vector3d<Timeseries_t> get_acceleration() const { return {base_type::_du, base_type::_dv, 0.0}; }

    //! Get the chassis angular acceleration
    Vector3d<Timeseries_t> get_angles_acceleration() const { return {0.0, 0.0, base_type::_dOmega}; }

    //! Computes the front axle velocity in chassis frame. Zero in the 3DOF model
    Vector3d<Timeseries_t> get_front_axle_velocity() const { return {0.0, 0.0, 0.0}; }

    //! Computes the rear axle velocity in chassis frame. Zero in the 3DOF model
    Vector3d<Timeseries_t> get_rear_axle_velocity() const { return {0.0, 0.0, 0.0}; }

    //! Get a negative normal force
    const Timeseries_t& get_negative_normal_force(const Algebraic id) const
    {
        switch(id)
        {
         case (IFZFL): return _neg_Fz_fl; break; 
         case (IFZFR): return _neg_Fz_fr; break; 
         case (IFZRL): return _neg_Fz_rl; break; 
         case (IFZRR): return _neg_Fz_rr; break; 
         default:      throw fastest_lap_exception("Id is incorrect");
        }
    }

    //! Get the throttle
    const Timeseries_t& get_throttle() const { return _throttle; }

    //! Get the brake bias
    const Timeseries_t& get_brake_bias() const { return _brake_bias; }

    //! Return a mechanical/geometrical parameter by name
    //! @param[in] parameter_name: name of the parameter
    scalar get_parameter(const std::string& parameter_name) const;

    //! Load the time derivative of the state variables computed herein to the dqdt
    //! @param[out] dqdt: the vehicle state vector time derivative
    template<size_t N>
    void get_state_derivative(std::array<Timeseries_t,N>& dqdt) const;

    //! Load the algebraic constraints computed herein to the dqa
    //! @param[out] dqa: the algebraic constraints
    template<size_t NALGEBRAIC_>
    void get_algebraic_constraints(std::array<Timeseries_t,NALGEBRAIC_>& dqa) const;

    //! Set the state variables of this class
    //! @param[in] q: the vehicle state vector 
    //! @param[in] u: the vehicle control vector
    template<size_t NSTATE, size_t NCONTROL>
    void set_state_and_controls(const std::array<Timeseries_t,NSTATE>& q, 
                                const std::array<Timeseries_t,NALGEBRAIC>& qa,
                                const std::array<Timeseries_t,NCONTROL>& u);


    //! Set the state and controls upper, lower, and default values
    template<size_t NSTATE, size_t NCONTROL>
    void set_state_and_control_upper_lower_and_default_values(std::array<scalar,NSTATE>& q_def,
                                                               std::array<scalar,NSTATE>& q_lb,
                                                               std::array<scalar,NSTATE>& q_ub,
                                                               std::array<scalar,NALGEBRAIC>& qa_def,
                                                               std::array<scalar,NALGEBRAIC>& qa_lb,
                                                               std::array<scalar,NALGEBRAIC>& qa_ub,
                                                               std::array<scalar,NCONTROL>& u_def,
                                                               std::array<scalar,NCONTROL>& u_lb,
                                                               std::array<scalar,NCONTROL>& u_ub 
                                                              ) const;

    //! Get the names of the state and control varaibles of this class
    //! @param[out] q: the vehicle state names
    //! @param[out] u: the vehicle control names
    template<size_t NSTATE, size_t NCONTROL>
    static void set_state_and_control_names(std::array<std::string,NSTATE>& q, 
                                     std::array<std::string,NALGEBRAIC>& qa,
                                     std::array<std::string,NCONTROL>& u);

    static std::string type() { return "chassis_car_3dof"; }


 private:

    // Geometrical properties
    Vector3d<Timeseries_t> _x_com;              //! [c] Center of mass position[m]
    Vector3d<Timeseries_t> _x_front_axle;       //! [c] Center of front axle [m]
    Vector3d<Timeseries_t> _x_rear_axle;        //! [c] Center of the rear axle [m]
    Vector3d<Timeseries_t> _x_aero;             //! [c] Aerodynamic center [m]

    // Mechanical properties
    Timeseries_t   _roll_balance_coeff; //! [c] Coefficient in [0,1], usually 1/2, such that: Fz_fr − Fz_fl = D(Fz_fr + Fz_rr − Fz_fl − Fz_rl)
    scalar         _Fz_max_ref2;        //! [c] Square of the parasitic smooth positive force when Fz = 0

    // Controllability properties
    scalar  _maximum_throttle;

    // Variables    ----------------------------------------------------------------

    // Setteable variables
    Timeseries_t _brake_bias_0; //! [in] Initial value for the brake bias (to be read from database)

    // Control variables
    Timeseries_t _throttle;     //! [in] throttle: 1-full throttle, -1-hard brake
    Timeseries_t _brake_bias;   //! [in] Brake bias: 1-only front, 0-only rear

    // Algebraic variables
    Timeseries_t _Fz_fl;    //! [in] Vertical load for FL tire
    Timeseries_t _Fz_fr;    //! [in] Vertical load for FR tire
    Timeseries_t _Fz_rl;    //! [in] Vertical load for RL tire
    Timeseries_t _Fz_rr;    //! [in] Vertical load for RR tire

    Timeseries_t _neg_Fz_fl;    //! negative vertical load for FL tire
    Timeseries_t _neg_Fz_fr;    //! negative vertical load for FR tire
    Timeseries_t _neg_Fz_rl;    //! negative vertical load for RL tire
    Timeseries_t _neg_Fz_rr;    //! negative vertical load for RR tire

    // Algebraic constraints
    Timeseries_t _Fz_eq;            //! [out] vertical equilibrium equation
    Timeseries_t _Mx_eq;            //! [out] roll equilibrium equation
    Timeseries_t _My_eq;            //! [out] pitch equilibrium equation
    Timeseries_t _roll_balance_eq;  //! [out] roll balance equation

    DECLARE_PARAMS(
        { "com/x", _x_com.x() },
        { "com/y", _x_com.y() },
        { "com/z", _x_com.z() },
        { "front_axle/x", _x_front_axle.x() },
        { "front_axle/y", _x_front_axle.y() },
        { "front_axle/z", _x_front_axle.z() },
        { "rear_axle/x", _x_rear_axle.x() },
        { "rear_axle/y", _x_rear_axle.y() },
        { "rear_axle/z", _x_rear_axle.z() }, 
        { "pressure_center/x", _x_aero.x() },
        { "pressure_center/y", _x_aero.y() },
        { "pressure_center/z", _x_aero.z() },
        { "brake_bias", _brake_bias_0 },
        { "roll_balance_coefficient", _roll_balance_coeff},
        { "Fz_max_ref2", _Fz_max_ref2 },
        { "maximum_throttle", _maximum_throttle }
    );  
};

#include "chassis_car_3dof.hpp"

#endif
