#ifndef CYCLUS_REQUEST_H_
#define CYCLUS_REQUEST_H_

#include <ostream>
#include <string>

#include <boost/shared_ptr.hpp>

#include "cyc_limits.h"

namespace cyclus {

class Trader;
template <class T> class RequestPortfolio;
    
/// A Request encapsulates all the information required to communicate the
/// needs of an agent in the Dynamic Resource Exchange, including the
/// commodity it needs as well as a resource specification for that commodity.
/// A Request is templated its resource.
template <class T>
struct Request {
 public:
  typedef boost::shared_ptr< Request<T> > Ptr;

  Request() : id_(next_id_++) { };
  
  /// @return the commodity associated with this request
  std::string commodity;

  /// @return the target resource for this request
  boost::shared_ptr<T> target;

  /// @return the preference value for this request
  double preference;

  /// @return the model requesting the resource
  Trader* requester;
  
  /// @return the portfolio of which this request is a part
  RequestPortfolio<T>* portfolio;
  
  /// @return a unique id for the request
  const int id() const {return id_;};

  /* -------------------- private methods and members -------------------------- */
  int id_;
  static int next_id_;
};

template<class T> int Request<T>::next_id_ = 0;

/// @brief equality operator
template<class T>
inline bool operator==(const Request<T>& lhs,
                       const Request<T>& rhs) {
  return  ((lhs.commodity == rhs.commodity) &&
           (lhs.target == rhs.target) &&
           (DoubleEq(lhs.preference, rhs.preference)) &&
           (lhs.requester == rhs.requester));
}

/// @brief comparison operator, allows usage in ordered containers
template<class T>
inline bool operator<(const cyclus::Request<T>& lhs,
                      const cyclus::Request<T>& rhs) {
  return  (lhs.id() < rhs.id());
};

} // namespace cyclus

#endif
