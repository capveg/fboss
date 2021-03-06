/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/sai/api/SaiApiTable.h"
#include "fboss/agent/hw/sai/fake/FakeSai.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiNeighborManager.h"
#include "fboss/agent/hw/sai/switch/SaiNextHopGroupManager.h"
#include "fboss/agent/hw/sai/switch/tests/ManagerTestBase.h"
#include "fboss/agent/state/RouteNextHopEntry.h"
#include "fboss/agent/types.h"

using namespace facebook::fboss;

/*
 * In these tests, we will assume 4 ports with one lane each, with IDs
 */

class NextHopGroupManagerTest : public ManagerTestBase {
 public:
  void SetUp() override {
    setupStage = SetupStage::PORT | SetupStage::VLAN | SetupStage::INTERFACE;
    ManagerTestBase::SetUp();
    intf0 = testInterfaces[0];
    h0 = intf0.remoteHosts[0];
    intf1 = testInterfaces[1];
    h1 = intf1.remoteHosts[0];
  }

  void checkNextHopGroup(
      sai_object_id_t nextHopGroupId,
      const std::unordered_set<folly::IPAddress>& expectedNextHopIps) {
    auto& nextHopGroupApi = saiApiTable->nextHopGroupApi();
    auto& nextHopApi = saiApiTable->nextHopApi();
    auto size = expectedNextHopIps.size();
    std::vector<sai_object_id_t> m;
    m.resize(expectedNextHopIps.size());
    NextHopGroupApiParameters::Attributes::NextHopMemberList memberList{m};
    auto members = nextHopGroupApi.getAttribute(memberList, nextHopGroupId);
    EXPECT_EQ(members.size(), size);

    std::unordered_set<folly::IPAddress> gotNextHopIps;
    for (const auto& member : members) {
      sai_object_id_t nextHopId = nextHopGroupApi.getMemberAttribute(
          NextHopGroupApiParameters::MemberAttributes::NextHopId{}, member);
      folly::IPAddress ip = nextHopApi.getAttribute(
          NextHopApiParameters::Attributes::Ip{}, nextHopId);
      EXPECT_TRUE(gotNextHopIps.insert(ip).second);
    }
    EXPECT_EQ(gotNextHopIps, expectedNextHopIps);
  }

  TestInterface intf0;
  TestRemoteHost h0;
  TestInterface intf1;
  TestRemoteHost h1;
};

TEST_F(NextHopGroupManagerTest, addNextHopGroup) {
  ResolvedNextHop nh1{h0.ip, InterfaceID(intf0.id), ECMP_WEIGHT};
  ResolvedNextHop nh2{h1.ip, InterfaceID(intf1.id), ECMP_WEIGHT};
  RouteNextHopEntry::NextHopSet swNextHops{nh1, nh2};
  auto saiNextHopGroup =
      saiManagerTable->nextHopGroupManager().incRefOrAddNextHopGroup(
          swNextHops);
  EXPECT_EQ(saiNextHopGroup.use_count(), 1);
  checkNextHopGroup(saiNextHopGroup->id(), {});
}

TEST_F(NextHopGroupManagerTest, refNextHopGroup) {
  ResolvedNextHop nh1{h0.ip, InterfaceID(intf0.id), ECMP_WEIGHT};
  ResolvedNextHop nh2{h1.ip, InterfaceID(intf1.id), ECMP_WEIGHT};
  RouteNextHopEntry::NextHopSet swNextHops{nh1, nh2};
  auto saiNextHopGroup =
      saiManagerTable->nextHopGroupManager().incRefOrAddNextHopGroup(
          swNextHops);
  EXPECT_EQ(saiNextHopGroup.use_count(), 1);

  RouteNextHopEntry::NextHopSet swNextHops2{nh1, nh2};
  auto saiNextHopGroup2 =
      saiManagerTable->nextHopGroupManager().incRefOrAddNextHopGroup(
          swNextHops2);
  EXPECT_EQ(saiNextHopGroup.use_count(), 2);

  EXPECT_EQ(saiNextHopGroup, saiNextHopGroup2);
  checkNextHopGroup(saiNextHopGroup->id(), {});
}

TEST_F(NextHopGroupManagerTest, derefNextHopGroup) {
  ResolvedNextHop nh1{h0.ip, InterfaceID(intf0.id), ECMP_WEIGHT};
  ResolvedNextHop nh2{h1.ip, InterfaceID(intf1.id), ECMP_WEIGHT};
  RouteNextHopEntry::NextHopSet swNextHops{nh1, nh2};
  auto saiNextHopGroup =
      saiManagerTable->nextHopGroupManager().incRefOrAddNextHopGroup(
          swNextHops);
  EXPECT_EQ(saiNextHopGroup.use_count(), 1);
  {
    RouteNextHopEntry::NextHopSet swNextHops2{nh1, nh2};
    auto saiNextHopGroup2 =
        saiManagerTable->nextHopGroupManager().incRefOrAddNextHopGroup(
            swNextHops2);
    EXPECT_EQ(saiNextHopGroup.use_count(), 2);
  }
  EXPECT_EQ(saiNextHopGroup.use_count(), 1);
  checkNextHopGroup(saiNextHopGroup->id(), {});
}

TEST_F(NextHopGroupManagerTest, deleteNextHopGroup) {
  std::weak_ptr<SaiNextHopGroup> counter;
  {
    ResolvedNextHop nh1{h0.ip, InterfaceID(intf0.id), ECMP_WEIGHT};
    ResolvedNextHop nh2{h1.ip, InterfaceID(intf1.id), ECMP_WEIGHT};
    RouteNextHopEntry::NextHopSet swNextHops{nh1, nh2};
    auto saiNextHopGroup =
        saiManagerTable->nextHopGroupManager().incRefOrAddNextHopGroup(
            swNextHops);
    counter = saiNextHopGroup;
    EXPECT_EQ(counter.use_count(), 1);
  }
  EXPECT_EQ(counter.use_count(), 0);
  EXPECT_TRUE(counter.expired());
}

TEST_F(NextHopGroupManagerTest, resolveNeighborBefore) {
  auto arpEntry0 = makeArpEntry(intf0.id, h0);
  saiManagerTable->neighborManager().addNeighbor(arpEntry0);
  auto arpEntry1 = makeArpEntry(intf1.id, h1);
  saiManagerTable->neighborManager().addNeighbor(arpEntry1);
  ResolvedNextHop nh1{h0.ip, InterfaceID(intf0.id), ECMP_WEIGHT};
  ResolvedNextHop nh2{h1.ip, InterfaceID(intf1.id), ECMP_WEIGHT};
  RouteNextHopEntry::NextHopSet swNextHops{nh1, nh2};
  auto saiNextHopGroup =
      saiManagerTable->nextHopGroupManager().incRefOrAddNextHopGroup(
          swNextHops);
  checkNextHopGroup(saiNextHopGroup->id(), {h0.ip, h1.ip});
}

TEST_F(NextHopGroupManagerTest, resolveNeighborAfter) {
  ResolvedNextHop nh1{h0.ip, InterfaceID(intf0.id), ECMP_WEIGHT};
  ResolvedNextHop nh2{h1.ip, InterfaceID(intf1.id), ECMP_WEIGHT};
  RouteNextHopEntry::NextHopSet swNextHops{nh1, nh2};
  auto saiNextHopGroup =
      saiManagerTable->nextHopGroupManager().incRefOrAddNextHopGroup(
          swNextHops);
  checkNextHopGroup(saiNextHopGroup->id(), {});
  /*
   * will pass in D13604057
   * auto arpEntry0 = makeArpEntry(intf0.id, h0);
   * saiManagerTable->neighborManager().addNeighbor(arpEntry0);
   * auto arpEntry1 = makeArpEntry(intf1.id, h1);
   * saiManagerTable->neighborManager().addNeighbor(arpEntry1);
   * checkNextHopGroup(saiNextHopGroup->id(), {h0.ip, h1.ip});
   */
}

TEST_F(NextHopGroupManagerTest, unresolveNeighbor) {}

TEST_F(NextHopGroupManagerTest, derefThenResolve) {}

TEST_F(NextHopGroupManagerTest, deleteThenResolve) {}

TEST_F(NextHopGroupManagerTest, l3UnresolvedNextHop) {}
