#ifndef CYCLUS_SRC_TOOLKIT_RES_MAP_H_
#define CYCLUS_SRC_TOOLKIT_RES_MAP_H_

#include <iomanip>
#include <limits>
#include <list>
#include <set>
#include <vector>

#include "cyc_arithmetic.h"
#include "cyc_limits.h"
#include "error.h"
#include "product.h"
#include "material.h"
#include "resource.h"
#include "res_manip.h"

namespace cyclus {
namespace toolkit {

/// ResMap     is a helper class that provides semi-automated management of
/// a collection of resources (e.g. agent stocks and inventories).
/// Constructed buffers have infinite capacity unless explicitly changed.
/// Resource popping occurs in the order the resources were pushed (i.e. oldest
/// resources are popped first), unless explicitly specified otherwise.
///
/// Typically, a ResMap     will be a member variable on an agent/archetype class.
/// Resources can be added and retrieved from it as needed, and the buffer can
/// be queried in various ways as done in the example below:
///
/// @code
/// class MyAgent : public cyclus::Facility {
///  public:
///   Tick() {
///     double batch_size = 2703;
///     if (outventory_.space() < batch_size) {
///       return;
///     } else if (inventory_.quantity() < batch_size) {
///       return;
///     }
///
///     outventory_.Push(inventory_.Pop(batch_size));
///   }
///
///   ... // resource exchange to fill up inventory_ buffer
///
///  private:
///   ...
///   cyclus::toolkit::ResMap    <cyclus::Material> inventory_;
///   cyclus::toolkit::ResMap    <cyclus::Material> outventory_;
/// };
/// @endcode
///
/// In this example, if there is sufficient material in inventory_, 2703 kg is
/// removed as a single object that is then placed in another buffer
/// (outventory_) each time step.
template <class K, class R>
class ResMap {
 public:
  ResMap() : capacity_(INFINITY), dirty_quantity_(true), quantity_(0) {
    Warn<EXPERIMENTAL_WARNING>("ResMap is experimental and its API may be "
                               "subject to change");
  }

  virtual ~ResMap() {}

  typedef std::map<typename K, typename R::Ptr>::iterator iterator;
  typedef std::map<typename K, typename R::Ptr>::const_iterator const_iterator;

  /// Returns the maximum resource quantity this buffer can hold (units
  /// based on constituent resource objects' units).
  /// Never throws.
  inline double capacity() const { return capacity_; }

  /// Sets the maximum quantity this buffer can hold (units based
  /// on constituent resource objects' units).
  ///
  /// @throws ValueError the new capacity is lower (by eps_rsrc()) than the
  /// quantity of resources that exist in the buffer.
  void capacity(double cap) {
    if (quantity() - cap > eps_rsrc()) {
      std::stringstream ss;
      ss << std::setprecision(17) << "new capacity " << cap
         << " lower than existing quantity " << quantity();
      throw ValueError(ss.str());
    }
    capacity_ = cap;
  }

  /// Returns the total number of resources in the map.
  inline int size() const { return resources_.size(); }

  /// Returns the total resource quantity of constituent resource
  /// objects in the map.
  inline double quantity() const {
    if (dirty_quantity_)
      UpdateQuantity();
    return quantity_;
  };

  /// Returns the quantity of space remaining in this buffer.
  /// This is effectively the difference between the capacity and the quantity
  /// and is never negative. Never throws.
  inline double space() const { return std::max(0.0, capacity_ - quantity()); }

  /// Returns true if there are no resources in the buffer.
  inline bool empty() const { return resources_.empty(); }

  typename R:Ptr operator[](const typename K& k) {
    dirty_quantity_ = true;
    return resources_[k];
  };

  const typename R:Ptr operator[](const typename K& k) {
    dirty_quantity_ = true;
    return const_cast<std::map<typename K, typename R::Ptr>&>(resources_)[k];
  };

  iterator begin() {
    dirty_quantity_ = true;
    return resources_.begin();
  }
  const_iterator begin() const { return resources_.begin(); }
  const_iterator cbegin() const { return resources_.begin(); }

  iterator end() {
    dirty_quantity_ = true;
    return resources_.end();
  }
  const_iterator end() const { return resources_.end(); }
  const_iterator cend() const { return resources_.end(); }

  void erase(iterator position) {
    resources_.erase(position);
    UpdateQuantity();
  };
  size_type erase(const typename K& k) {
    size_type s = resources_.erase(k);
    UpdateQuantity();
    return s;
  };
  void erase(iterator first, iterator last) {
    resources_.erase(first, last);
    UpdateQuantity();
  };

 private:
  void UpdateQuantity() {
    using std::vector;
    typename std::list<typename K, typename R::Ptr>::iterator it;
    int n = resources_.size();
    int i;
    vector<double> qtys (n, 0.0);
    it = resources_.begin();
    while (it != resources_.end()) {
      qtys[i] = (*it)->quantity();
      ++i;
      ++it;
    }
    quantity_ = CycArithmetic::KahanSum(qtys);
    dirty_quantity_ = false;
  }

  /// Maximum quantity of resources this mapping can hold.
  double capacity_;

  /// Whether quantity_ should be recomputed or not.
  bool dirty_quantity_;

  /// Current total quantity of all resources in the mapping.
  double quantity_;

  /// Underlying container
  std::map<typename K, typename R::Ptr> resources_;
  std::set<typename R::Ptr> resources_present_;
};

}  // namespace toolkit
}  // namespace cyclus

#endif  // CYCLUS_SRC_TOOLKIT_RES_MAP_H_
