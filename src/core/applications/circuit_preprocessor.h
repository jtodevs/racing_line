#ifndef __CIRCUIT_PREPROCESSOR_H__
#define __CIRCUIT_PREPROCESSOR_H__

#include <vector>
#include <array>
#include "lion/math/vector3d.h"
#include "lion/io/Xml_document.h"
#include <memory>


class Circuit_preprocessor
{
 public:

    constexpr const static int CLOCKWISE = -1;
    constexpr const static int COUNTERCLOCKWISE = 1;

    struct Coordinates
    {
        scalar longitude;
        scalar latitude;
    };

    struct Options
    {
        // Fitness function cost parameters
        scalar eps_d = 1.0e-1;
        scalar eps_k = 5.0e4;
        scalar eps_n = 1.0e-1;
        scalar eps_c = 1.0e-1;

        scalar maximum_kappa = 0.1;
        scalar maximum_dkappa = 2.0e-2;
        scalar maximum_dn     = 1.0;
        scalar maximum_distance_find = 50.0;

        scalar adaption_aspect_ratio_max = 1.2;

        int print_level = 0;
    };

    //! Default constructor
    Circuit_preprocessor() = default;

    //! Constructor from Xml file
    Circuit_preprocessor(Xml_document& doc);

    //! Any constructor from KML
    template<typename ... Args>
    Circuit_preprocessor(Xml_document& coord_left_kml, 
                         Xml_document& coord_right_kml,
                         bool clockwise,
                         Options opts,
                         Args&&... args)
    {
        // (1) Read the KML files and transform to vector of coordinates
        auto [coord_left, coord_right] = read_kml(coord_left_kml,coord_right_kml, clockwise);

        // (2) Call the proper implementation from vector of coordinates 
        *this = Circuit_preprocessor(coord_left, coord_right, opts, std::forward<Args>(args)...);
    }

    //! Constructor for closed circuits, from number of elements
    Circuit_preprocessor(const std::vector<Coordinates>& coord_left, 
                         const std::vector<Coordinates>& coord_right, 
                         const Options opts, 
                         const size_t n_el) 
    : options(opts), n_elements(n_el), n_points(n_el), is_closed(true), direction(0)
    {
        // (1) Compute the centerline and preprocess inputs
        transform_coordinates<true>(coord_left, coord_right);

        // (3) Compute the centerline estimate
        const auto [s_center,r_center,track_length_estimate] = compute_averaged_centerline<true>(r_left_measured,r_right_measured,n_elements,n_points,options);

        // (3) Perform the optimization
        compute<true>(s_center, r_center, track_length_estimate);
    }

    //! Constructor for closed circuits, from mesh size given as breakpoints along the circuit
    Circuit_preprocessor(const std::vector<Coordinates>& coord_left, 
                         const std::vector<Coordinates>& coord_right, 
                         const Options opts, 
                         const std::vector<std::pair<Coordinates,scalar>>& ds_breakpoints) 
    : options(opts), n_elements(0), n_points(0), is_closed(true), direction(0)
    {
        // (1) Compute the centerline and preprocess inputs
        transform_coordinates<true>(coord_left, coord_right);

        // transform the ds_breakpoints to sVector
        std::vector<std::pair<sVector3d,scalar>> ds_breakpoints_v3d(ds_breakpoints.size());

        for (size_t i = 0; i < ds_breakpoints.size(); ++i)
        {
            ds_breakpoints_v3d[i].first = sVector3d((ds_breakpoints[i].first.longitude*DEG-theta0)*R_earth*cos(phi_ref), 
                                                    (ds_breakpoints[i].first.latitude*DEG-phi0)*R_earth, 
                                                    0.0);
            ds_breakpoints_v3d[i].second = ds_breakpoints[i].second;
        }

        // (3) Compute the centerline estimate
        const auto [s_center,r_center,track_length_estimate] = compute_averaged_centerline<true>(r_left_measured,r_right_measured,ds_breakpoints_v3d,options);

        n_points   = s_center.size();
        n_elements = n_points;

        // (3) Perform the optimization
        compute<true>(s_center, r_center, track_length_estimate);
    }

    //! Constructor for closed circuits, from mesh size given as breakpoints along the circuit
    Circuit_preprocessor(const std::vector<Coordinates>& coord_left, 
                         const std::vector<Coordinates>& coord_right, 
                         const Options opts, 
                         const std::vector<scalar>& s_distribution,
                         const std::vector<scalar>& ds_distribution)
    : options(opts), n_elements(0), n_points(0), is_closed(true), direction(0)
    {
        // (1) Compute the centerline and preprocess inputs
        transform_coordinates<true>(coord_left, coord_right);

        // (2) Compute the centerline estimate
        const auto [s_center,r_center,track_length_estimate] = compute_averaged_centerline<true>(r_left_measured,r_right_measured,s_distribution,ds_distribution,options);

        n_points   = s_center.size();
        n_elements = n_points;

        // (3) Perform the optimization
        compute<true>(s_center, r_center, track_length_estimate);
    }



    //! Constructor for open circuits
    Circuit_preprocessor(const std::vector<Coordinates>& coord_left, 
                         const std::vector<Coordinates>& coord_right, 
                         const Options opts, 
                         Coordinates start, 
                         Coordinates finish, 
                         const size_t n_el) 
    : options(opts), n_elements(n_el), n_points(n_el+1), is_closed(false), direction(0)
    {
        // (1) Trim the coordinates to the provided start/finish points
        auto [coord_left_trim, coord_right_trim] = trim_coordinates(coord_left, coord_right, start, finish);

        // (2) Compute the centerline and preprocess inputs
        transform_coordinates<false>(coord_left_trim, coord_right_trim);

        // (3) Compute the centerline estimate
        const auto [s_center,r_center,track_length_estimate] = compute_averaged_centerline<false>(r_left_measured,r_right_measured,n_elements,n_points,options);

        // (4) Perform the optimization
        compute<false>(s_center, r_center, track_length_estimate);
    }

    // Inputs ------------------------------------:-
    Options     options;
    size_t      n_elements;
    size_t      n_points;
    bool        is_closed;
    int         direction;      

    // Outputs -----------------------------------:-
    scalar x0;      
    scalar y0;      
    scalar phi0;    
    scalar theta0;  
    scalar phi_ref; 
    scalar R_earth = 6378388.0;

    std::vector<sVector3d> r_left;
    std::vector<sVector3d> r_left_measured;     
    std::vector<sVector3d> r_right;
    std::vector<sVector3d> r_right_measured;    
    std::vector<sVector3d> r_centerline;

    std::vector<scalar> s;
    std::vector<scalar> theta;
    std::vector<scalar> kappa;
    std::vector<scalar> nl;
    std::vector<scalar> nr;
    std::vector<scalar> dkappa;
    std::vector<scalar> dnl;
    std::vector<scalar> dnr;

    scalar track_length;
    scalar left_boundary_max_error;
    scalar right_boundary_max_error;
    scalar left_boundary_L2_error;
    scalar right_boundary_L2_error;

    std::unique_ptr<Xml_document> xml() const;

 private:

    template<bool closed>
    void transform_coordinates(const std::vector<Coordinates>& coord_left, const std::vector<Coordinates>& coord_right);

    template<bool closed>
    void compute(const std::vector<scalar>& s_center, const std::vector<sVector3d>& r_center, const scalar track_length_estimate);

    template<bool closed>
    class FG
    {
     public:
        using ADvector = std::vector<CppAD::AD<scalar>>;

        enum State { IX, IY, ITHETA, IKAPPA, INL, INR, NSTATE };
        enum Controls { IDKAPPA, IDNL, IDNR, NCONTROLS };

        FG(const size_t n_elements, 
           const size_t n_points,
           const std::vector<scalar>& element_ds,
           const std::vector<sVector3d>& r_left, 
           const std::vector<sVector3d>& r_right, 
           const std::vector<sVector3d>& r_center, 
           int direction, 
           const Options opts) 
            : _n_elements(n_elements), _n_points(n_points), _n_variables(1+(NSTATE+NCONTROLS)*_n_points), 
              _n_constraints(1+NSTATE*n_elements + (closed ? 0 : 1)), _direction(direction), options(opts), _ds(element_ds), _r_left(r_left), _r_right(r_right), _r_center(r_center), _q(_n_points), _u(_n_points), _dqds(_n_points),
              _dist2_left(_n_points), _dist2_right(_n_points), _dist2_center(_n_points) {}

        void operator()(ADvector& fg, const ADvector& x);

        std::array<CppAD::AD<scalar>,NSTATE> equations(const std::array<CppAD::AD<scalar>,NSTATE>& q, const std::array<CppAD::AD<scalar>,NCONTROLS>& u) const
        {
            return { cos(q[ITHETA]), sin(q[ITHETA]), q[IKAPPA], u[IDKAPPA], u[IDNL], u[IDNR] };
        }

        constexpr const size_t& get_n_points() const { return _n_points; }
        constexpr const size_t& get_n_variables() const { return _n_variables; }
        constexpr const size_t& get_n_constraints() const { return _n_constraints; }

     private:
        size_t _n_elements;
        size_t _n_points;
        size_t _n_variables;
        size_t _n_constraints;

        int _direction;

        Options options;
        std::vector<scalar> _ds;

        std::vector<sVector3d> _r_left;
        std::vector<sVector3d> _r_right;
        std::vector<sVector3d> _r_center;

        std::vector<std::array<CppAD::AD<scalar>,NSTATE>> _q;
        std::vector<std::array<CppAD::AD<scalar>,NCONTROLS>> _u;
        std::vector<std::array<CppAD::AD<scalar>,NSTATE>> _dqds;

        std::vector<CppAD::AD<scalar>> _dist2_left;
        std::vector<CppAD::AD<scalar>> _dist2_right;
        std::vector<CppAD::AD<scalar>> _dist2_center;
    };

    std::pair<std::vector<Coordinates>,std::vector<Coordinates>> read_kml(Xml_document& coord_left_kml, Xml_document& coord_right_kml, bool clockwise=false);

    //! Compute the averaged centerline between r_left and r_right with given number of elements
    template<bool closed>
    static std::tuple<std::vector<scalar>, std::vector<sVector3d>, scalar> compute_averaged_centerline(std::vector<sVector3d> r_left, 
                                                                                              std::vector<sVector3d> r_right, 
                                                                                              const size_t n_elements,
                                                                                              const size_t n_points,
                                                                                              const Options& options);
    
    //! Compute the averaged centerline between r_left and r_right with given arclength size breakpoints along the circuit
    template<bool closed>
    static std::tuple<std::vector<scalar>, std::vector<sVector3d>, scalar> compute_averaged_centerline(std::vector<sVector3d> r_left,
                                                                                             std::vector<sVector3d> r_right,
                                                                                             const std::vector<std::pair<sVector3d,scalar>>& ds_breakpoints,
                                                                                             const Options& options);

    //! Compute the averaged centerline between r_left and r_right with given arclength given as ds = f(s)
    template<bool closed>
    static std::tuple<std::vector<scalar>, std::vector<sVector3d>, scalar> compute_averaged_centerline(std::vector<sVector3d> r_left,
                                                                                              std::vector<sVector3d> r_right,
                                                                                              const std::vector<scalar>& s_distribution,
                                                                                              const std::vector<scalar>& ds_distribution,
                                                                                              const Options& options);



    static std::pair<std::vector<Coordinates>, std::vector<Coordinates>> trim_coordinates(const std::vector<Coordinates>& coord_left, 
                                                                                          const std::vector<Coordinates>& coord_right,
                                                                                          Coordinates start, Coordinates finish);

    template<bool closed>
    static scalar compute_ds_for_coordinates(const sVector3d point, const std::vector<sVector3d>& r_curve, const std::vector<std::pair<sVector3d,scalar>>& ds_breakpoints);

    static size_t who_is_ahead(std::array<size_t,2>& i_p1, std::array<size_t,2>& i_p2, const sVector3d& p1, const sVector3d& p2, const sVector3d& p_ref);
};

#include "circuit_preprocessor.hpp"

#endif
