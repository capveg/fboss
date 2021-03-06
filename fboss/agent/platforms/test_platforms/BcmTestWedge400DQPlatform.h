/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include "fboss/agent/platforms/test_platforms/BcmTestWedgeTomahawk3Platform.h"

namespace facebook {
namespace fboss {
class BcmTestWedge400DQPlatform : public BcmTestWedgeTomahawk3Platform {
 public:
  BcmTestWedge400DQPlatform();
  ~BcmTestWedge400DQPlatform() override {}

 private:
  // Forbidden copy constructor and assignment operator
  BcmTestWedge400DQPlatform(BcmTestWedge400DQPlatform const&) = delete;
  BcmTestWedge400DQPlatform& operator=(BcmTestWedge400DQPlatform const&) =
      delete;

  std::unique_ptr<BcmTestPort> createTestPort(PortID id) const override;
};
} // namespace fboss
} // namespace facebook
