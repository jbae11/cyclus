#include "enrichment_tests.h"

#include <cmath>

#include <gtest/gtest.h>

#include "composition.h"
#include "context.h"
#include "cyc_limits.h"
#include "env.h"
#include "error.h"
#include "material.h"
#include "recorder.h"
#include "timer.h"

namespace cyclus {
namespace toolkit {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EnrichmentTests::SetUp() {
  Env::SetNucDataPath();

  feed_ = 0.0072;
  product_ = 0.05;
  tails_ = 0.002;

  assay_u_ = product_;
  mass_u_ = 10;

  Timer ti;
  Recorder rec;
  Context ctx(&ti, &rec);

  CompMap v;
  v[922350000] = assay_u_;
  v[922380000] = 1 - assay_u_;
  Composition::Ptr comp1 = Composition::CreateFromMass(v);
  Composition::Ptr comp2 = Composition::CreateFromAtom(v);
  mat_by_mass_ = Material::CreateUntracked(mass_u_, comp1);
  mat_by_atom_ = Material::CreateUntracked(mass_u_, comp2);

  SetEnrichmentParameters();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EnrichmentTests::TearDown() {}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EnrichmentTests::SetEnrichmentParameters() {
  double feed_val = (1 - 2 * feed_) * std::log(1 / feed_ - 1);
  double tails_val = (1 - 2 * tails_) * std::log(1 / tails_ - 1);
  double product_val = (1 - 2 * product_) * std::log(1 / product_ - 1);

  feed_qty_ = mass_u_ * (product_ - tails_) / (feed_ - tails_);
  tails_qty_ = mass_u_ * (product_ - feed_) / (feed_ - tails_);
  swu_ = mass_u_ * product_val + tails_qty_ * tails_val - feed_qty_ * feed_val;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST_F(EnrichmentTests, assays) {
  Assays a(feed_, product_, tails_);
  EXPECT_DOUBLE_EQ(feed_, a.Feed());
  EXPECT_DOUBLE_EQ(product_, a.Product());
  EXPECT_DOUBLE_EQ(tails_, a.Tails());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST_F(EnrichmentTests, valuefunction) {
  EXPECT_THROW(ValueFunc(0 - eps()), ValueError);
  EXPECT_THROW(ValueFunc(1), ValueError);

  double step = 0.001;
  double test_value = 0;
  while (test_value < 1) {
    double expected = (1 - 2 * test_value) * std::log(1 / test_value - 1);
    EXPECT_DOUBLE_EQ(expected, ValueFunc(test_value));
    test_value += step;
  }
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST_F(EnrichmentTests, UraniumAssays) {
  EXPECT_DOUBLE_EQ(product_, UraniumAssayAtom(mat_by_atom_));
  EXPECT_DOUBLE_EQ(product_, UraniumAssayMass(mat_by_mass_));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST_F(EnrichmentTests, material) {
  EXPECT_DOUBLE_EQ(mass_u_, UraniumQty(mat_by_mass_));
  EXPECT_DOUBLE_EQ(mass_u_, UraniumQty(mat_by_atom_));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST_F(EnrichmentTests, enrichmentcalcs) {
  Assays assays(feed_, UraniumAssayMass(mat_by_mass_), tails_);
  double product_qty = UraniumQty(mat_by_mass_);
  EXPECT_DOUBLE_EQ(feed_qty_, FeedQty(product_qty, assays));
  EXPECT_DOUBLE_EQ(tails_qty_, TailsQty(product_qty, assays));
  EXPECT_NEAR(swu_, SwuRequired(product_qty, assays), 1e-8);
}

}  // namespace toolkit
}  // namespace cyclus
