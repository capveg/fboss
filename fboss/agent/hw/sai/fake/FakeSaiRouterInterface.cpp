/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "FakeSaiRouterInterface.h"
#include "FakeSai.h"

#include <folly/Optional.h>
#include <folly/logging/xlog.h>

using facebook::fboss::FakeRouterInterface;
using facebook::fboss::FakeSai;

sai_status_t create_router_interface_fn(
    sai_object_id_t* router_interface_id,
    sai_object_id_t /* switch_id */,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  auto fs = FakeSai::getInstance();
  folly::Optional<int32_t> type;
  folly::Optional<sai_object_id_t> vlanId;
  folly::Optional<sai_object_id_t> vrId;
  folly::Optional<folly::MacAddress> mac;
  for (int i = 0; i < attr_count; ++i) {
    switch (attr_list[i].id) {
      case SAI_ROUTER_INTERFACE_ATTR_SRC_MAC_ADDRESS:
        mac = folly::MacAddress::fromBinary(folly::ByteRange(
            std::begin(attr_list[i].value.mac),
            std::end(attr_list[i].value.mac)));
        break;
      case SAI_ROUTER_INTERFACE_ATTR_TYPE:
        type = attr_list[i].value.s32;
        break;
      case SAI_ROUTER_INTERFACE_ATTR_VIRTUAL_ROUTER_ID:
        vrId = attr_list[i].value.oid;
        break;
      case SAI_ROUTER_INTERFACE_ATTR_VLAN_ID:
        vlanId = attr_list[i].value.oid;
        break;
      default:
        return SAI_STATUS_INVALID_PARAMETER;
    }
  }
  if (vrId && type && vlanId) {
    *router_interface_id = fs->rim.create(
        FakeRouterInterface(vrId.value(), type.value(), vlanId.value()));
  } else {
    return SAI_STATUS_INVALID_PARAMETER;
  }
  if (mac) {
    auto& ri = fs->rim.get(*router_interface_id);
    ri.setSrcMac(mac.value());
  }
  return SAI_STATUS_SUCCESS;
}

sai_status_t remove_router_interface_fn(sai_object_id_t router_interface_id) {
  auto fs = FakeSai::getInstance();
  fs->rim.remove(router_interface_id);
  return SAI_STATUS_SUCCESS;
}

sai_status_t set_router_interface_attribute_fn(
    sai_object_id_t router_interface_id,
    const sai_attribute_t* attr) {
  auto fs = FakeSai::getInstance();
  auto& ri = fs->rim.get(router_interface_id);
  switch (attr->id) {
    case SAI_ROUTER_INTERFACE_ATTR_SRC_MAC_ADDRESS:
      ri.setSrcMac(attr->value.mac);
      break;
    default:
      return SAI_STATUS_INVALID_PARAMETER;
  }
  return SAI_STATUS_SUCCESS;
}

sai_status_t get_router_interface_attribute_fn(
    sai_object_id_t router_interface_id,
    uint32_t attr_count,
    sai_attribute_t* attr) {
  auto fs = FakeSai::getInstance();
  const auto& ri = fs->rim.get(router_interface_id);
  for (int i = 0; i < attr_count; ++i) {
    switch (attr[i].id) {
      case SAI_ROUTER_INTERFACE_ATTR_SRC_MAC_ADDRESS:
        std::copy_n(ri.srcMac().bytes(), 6, std::begin(attr[i].value.mac));
        break;
      case SAI_ROUTER_INTERFACE_ATTR_TYPE:
        attr[i].value.s32 = ri.type;
        break;
      case SAI_ROUTER_INTERFACE_ATTR_VIRTUAL_ROUTER_ID:
        attr[i].value.oid = ri.virtualRouterId;
        break;
      case SAI_ROUTER_INTERFACE_ATTR_VLAN_ID:
        attr[i].value.oid = ri.vlanId;
        break;
      default:
        return SAI_STATUS_INVALID_PARAMETER;
    }
  }
  return SAI_STATUS_SUCCESS;
}

namespace facebook {
namespace fboss {

static sai_router_interface_api_t _router_interface_api;

void populate_router_interface_api(
    sai_router_interface_api_t** router_interface_api) {
  _router_interface_api.create_router_interface = &create_router_interface_fn;
  _router_interface_api.remove_router_interface = &remove_router_interface_fn;
  _router_interface_api.set_router_interface_attribute =
      &set_router_interface_attribute_fn;
  _router_interface_api.get_router_interface_attribute =
      &get_router_interface_attribute_fn;
  *router_interface_api = &_router_interface_api;
}

} // namespace fboss
} // namespace facebook
