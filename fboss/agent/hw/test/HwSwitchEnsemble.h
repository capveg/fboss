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

#include "fboss/agent/HwSwitch.h"
#include "fboss/agent/types.h"

#include <memory>

namespace facebook {
namespace fboss {

class Platform;
class SwitchState;
class HwLinkStateToggler;

class HwSwitchEnsemble : public HwSwitch::Callback {
 public:
  class HwSwitchEventObserverIf {
   public:
    virtual ~HwSwitchEventObserverIf() = default;
    virtual void packetReceived(RxPacket* pkt) noexcept = 0;
    virtual void linkStateChanged(PortID port, bool up) = 0;
  };
  explicit HwSwitchEnsemble(uint32_t featuresDesired);
  ~HwSwitchEnsemble() override;
  /*
   * Revert back to post init state. Can be used by each
   * benchmark iteration to start with a clean slate.
   */
  void revertToInitCfgState();
  std::shared_ptr<SwitchState> applyNewState(
      std::shared_ptr<SwitchState> newState);
  void applyInitialConfigAndBringUpPorts(const cfg::SwitchConfig& cfg);

  std::shared_ptr<SwitchState> getProgrammedState() const;
  virtual HwLinkStateToggler* getLinkToggler() {
    return linkToggler_.get();
  }
  virtual Platform* getPlatform() {
    return platform_.get();
  }
  virtual HwSwitch* getHwSwitch() {
    return hwSwitch_.get();
  }
  void packetReceived(std::unique_ptr<RxPacket> pkt) noexcept override;
  void linkStateChanged(PortID port, bool up) override;
  void exitFatal() const noexcept override {}
  void addHwEventObserver(HwSwitchEventObserverIf* observer);
  void removeHwEventObserver(HwSwitchEventObserverIf* observer);

 protected:
  /*
   * Setup ensemble
   */
  void setupEnsemble(
      std::unique_ptr<Platform> platform,
      std::unique_ptr<HwSwitch> hwSwitch,
      std::unique_ptr<HwLinkStateToggler> linkToggler);

 private:
  std::shared_ptr<SwitchState> programmedState_;
  std::shared_ptr<SwitchState> initCfgState_;
  std::unique_ptr<HwLinkStateToggler> linkToggler_;
  std::unique_ptr<Platform> platform_;
  std::unique_ptr<HwSwitch> hwSwitch_;
  uint32_t featuresDesired_{0};
  std::set<HwSwitchEventObserverIf*> hwEventObservers_;
};

} // namespace fboss
} // namespace facebook
