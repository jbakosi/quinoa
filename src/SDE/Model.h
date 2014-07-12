//******************************************************************************
/*!
  \file      src/SDE/Model.h
  \author    J. Bakosi
  \date      Wed 19 Feb 2014 05:50:47 AM MST
  \copyright 2005-2014, Jozsef Bakosi.
  \brief     Model
  \details   Model
*/
//******************************************************************************
#ifndef Model_h
#define Model_h

#include <Types.h>
#include <Options/RNG.h>

namespace quinoa {

//! Model
class Model {

  public:
    //! Constructor
    explicit Model() = default;

    //! Advance particles
    virtual void advance(int p, int tid, tk::real dt) = 0;

    //! Destructor
    virtual ~Model() noexcept = default;

    //! Return true if model is stochastic (false if deterministic)
    virtual bool stochastic() const noexcept = 0;

    //! Return RNG type if stochastic, NO_RNG if deterministic
    virtual tk::ctr::RNGType rng() const noexcept = 0;

    //! Return number of components
    virtual int ncomp() const noexcept = 0;

    //! Return dependent variable
    virtual char depvar() const noexcept = 0;

    //! Return initialization policy
    virtual std::string initPolicy() const noexcept = 0;

    //! Return coefficients policy
    virtual std::string coeffPolicy() const noexcept = 0;

  private:
    //! Don't permit copy constructor
    Model(const Model&) = delete;
    //! Don't permit copy assigment
    Model& operator=(const Model&) = delete;
    //! Don't permit move constructor
    Model(Model&&) = delete;
    //! Don't permit move assigment
    Model& operator=(Model&&) = delete;
};

} // quinoa::

#endif // Model_h
